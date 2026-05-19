/**
 *******************************************************************************
 *  @file           compress_windows.c
 *  @brief          Windows 向け圧縮・展開モジュール (Compression API)。
 *  @author         Tetsuo Honda
 *  @date           2026/03/05
 *  @version        1.0.0
 *
 *  @details
 *  Windows 8 以降の Compression API を使用して raw DEFLATE ストリームを生成します。\n
 *
 *  - **圧縮**: `MSZIP | COMPRESS_RAW` (Block Mode) で圧縮し、先頭に付加される
 *    2 バイトの MSZIP ブロック署名 (CK = 0x43 0x4B) を除去して raw DEFLATE を得ます。
 *  - **展開**: `MSZIP | COMPRESS_RAW` の Decompress は常にエラー 605 を返す
 *    既知の問題があるため、代わりに MSZIP ストリーミング形式ヘッダーを手動構築して
 *    ストリーミング Decompressor に渡します。ヘッダーの CRC バイト (offset 6) は
 *    Windows Compression API の内部仕様に従い算出します。
 *
 *  これにより Linux 実装 (zlib, windowBits = -15) と同一の raw DEFLATE フォーマットを
 *  維持し、クロスプラットフォームに対応した圧縮・展開処理を行います。
 *
 *  @note           Cabinet.lib のリンクが必要です。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)

    #include <stdlib.h>
    #include <string.h>

    #include <com_util/base/windows_sdk.h>
    #include <compressapi.h>
    #pragma comment(lib, "Cabinet.lib")

    #include <com_util/compress/compress.h>

/*
 * 標準 CRC-32 (IEEE 802.3, polynomial 0xEDB88320) の継続計算。
 * prev: 前回の crc32 結果 (初回は任意の seed を渡す)。
 */
static uint32_t mszip_crc32(const uint32_t prev, const uint8_t *buf, const size_t len)
{
    uint32_t crc = ~prev;
    size_t remaining = len;
    while (remaining--)
    {
        int i;
        uint32_t b = *buf++;
        crc ^= b;
        for (i = 0; i < 8; i++)
            crc = (crc >> 1u) ^ (0xEDB88320u & (uint32_t)(-(int32_t)(crc & 1u)));
    }
    return ~crc;
}

/*
 * MSZIP ストリーミングヘッダーの offset 6 に格納する CRC バイトを計算する。
 *
 * 計算式 (Cabinet.dll より逆解析 / pymszip・bytewitch にて実証):
 *   seed   = CRC32( {0x0A,0x51,0xE5,0xC0,0x18,0x00} )  = 0xE73FDBAD (固定)
 *   byte6  = CRC32( {0x02, orig_len_LE8, orig_len_LE8}, seed ) & 0xFF
 *
 * 単一チャンク (orig_len <= 32768) の場合、total_len == first_chunk_len のため
 * orig_len を 2 回使用する。
 */
static uint8_t mszip_crc_byte(const uint64_t orig_len)
{
    /* seed = CRC32(magic_prefix) = 0xE73FDBAD */
    static const uint32_t SEED = 0xE73FDBADu;

    uint8_t data[17];
    uint32_t crc;

    data[0] = 0x02u; /* COMPRESS_ALGORITHM_MSZIP */
    memcpy(data + 1, &orig_len, 8);
    memcpy(data + 9, &orig_len, 8);

    crc = mszip_crc32(SEED, data, sizeof(data));
    return (uint8_t)(crc & 0xFFu);
}

/* doxygen コメントは、ヘッダーに記載 */
int com_util_compress(uint8_t *dst, size_t *dst_len, const uint8_t *src, const size_t src_len)
{
    COMPRESSOR_HANDLE h;
    uint32_t orig_len_nbo;
    SIZE_T cmp_len;
    ULONG block_size;
    BOOL ok;

    if (dst == NULL || dst_len == NULL || src == NULL || src_len == 0)
    {
        return -1;
    }

    /* NBO ヘッダー (4B) + CK (2B) + raw DEFLATE 最低 1B */
    if (*dst_len < COM_UTIL_COMPRESS_HEADER_SIZE + 2U + 1U)
    {
        return -1;
    }

    /* 先頭 4 バイトに元サイズ (NBO) を書く */
    orig_len_nbo = htonl((uint32_t)src_len);
    memcpy(dst, &orig_len_nbo, COM_UTIL_COMPRESS_HEADER_SIZE);

    /*
     * MSZIP|COMPRESS_RAW で圧縮する。
     * 出力は [CK(2B)][raw DEFLATE] の形式になるため、
     * CK プレフィックス 2B を除去して raw DEFLATE のみを保存する。
     */
    if (!CreateCompressor(COMPRESS_ALGORITHM_MSZIP | COMPRESS_RAW, NULL, &h))
    {
        return -1;
    }

    /* データ全体を単一ブロックとして処理する */
    if (src_len > 32768U)
    {
        block_size = (ULONG)src_len;
    }
    else
    {
        block_size = 32768U;
    }
    (void)SetCompressorInformation(h, COMPRESS_INFORMATION_CLASS_BLOCK_SIZE, &block_size, sizeof(block_size));

    /* dst+4 に [CK(2B)][raw DEFLATE] を書き込む */
    ok = Compress(h, src, (SIZE_T)src_len, dst + COM_UTIL_COMPRESS_HEADER_SIZE,
                  (SIZE_T)(*dst_len - COM_UTIL_COMPRESS_HEADER_SIZE), &cmp_len);
    CloseCompressor(h);

    if (!ok || cmp_len < 2U)
    {
        return -1;
    }

    /* CK(2B) を除去: [CK][raw DEFLATE] → [raw DEFLATE] */
    memmove(dst + COM_UTIL_COMPRESS_HEADER_SIZE, dst + COM_UTIL_COMPRESS_HEADER_SIZE + 2U, (size_t)(cmp_len - 2U));

    *dst_len = COM_UTIL_COMPRESS_HEADER_SIZE + (size_t)(cmp_len - 2U);
    return 0;
}

/* doxygen コメントは、ヘッダーに記載 */
int com_util_decompress(uint8_t *dst, size_t *dst_len, const uint8_t *src, const size_t src_len)
{
    DECOMPRESSOR_HANDLE h;
    uint32_t orig_len_nbo;
    uint64_t orig_len;
    size_t raw_deflate_len;
    size_t tmp_len;
    uint8_t *tmp;
    SIZE_T out_len;
    uint32_t chunk_size;
    BOOL ok;

    if (dst == NULL || dst_len == NULL || src == NULL || src_len <= COM_UTIL_COMPRESS_HEADER_SIZE)
    {
        return -1;
    }

    /* 先頭 4 バイトから元サイズを取得 */
    memcpy(&orig_len_nbo, src, COM_UTIL_COMPRESS_HEADER_SIZE);
    orig_len = (uint64_t)ntohl(orig_len_nbo);

    if (*dst_len < (size_t)orig_len)
    {
        return -1;
    }

    raw_deflate_len = src_len - COM_UTIL_COMPRESS_HEADER_SIZE;

    /*
     * MSZIP|COMPRESS_RAW の Decompress は常にエラー 605 を返すため、
     * MSZIP ストリーミング形式のヘッダーを手動構築して
     * ストリーミング Decompressor に渡す。
     *
     * ストリーミング MSZIP フォーマット (単一チャンク):
     *   [24B header] [4B chunk_size] [2B CK] [raw DEFLATE]
     */
    tmp_len = 24U + 4U + 2U + raw_deflate_len;
    tmp = (uint8_t *)malloc(tmp_len);
    if (tmp == NULL)
    {
        return -1;
    }

    /* ヘッダー: magic (4B) + header_size (2B) + crc_byte (1B) + algo_id (1B) */
    tmp[0] = 0x0Au;
    tmp[1] = 0x51u;
    tmp[2] = 0xE5u;
    tmp[3] = 0xC0u;
    tmp[4] = 0x18u;
    tmp[5] = 0x00u;
    tmp[6] = mszip_crc_byte(orig_len);
    tmp[7] = 0x02u; /* COMPRESS_ALGORITHM_MSZIP */

    /* total_decompressed_len (8B LE) × 2 */
    memcpy(tmp + 8, &orig_len, 8);
    memcpy(tmp + 16, &orig_len, 8);

    /* chunk_size: CK(2B) + raw DEFLATE のサイズ */
    chunk_size = (uint32_t)(raw_deflate_len + 2U);
    memcpy(tmp + 24, &chunk_size, 4);

    /* CK プレフィックス */
    tmp[28] = 0x43u;
    tmp[29] = 0x4Bu;

    /* raw DEFLATE データ */
    memcpy(tmp + 30, src + COM_UTIL_COMPRESS_HEADER_SIZE, raw_deflate_len);

    /* MSZIP ストリーミング Decompressor で展開 */
    if (!CreateDecompressor(COMPRESS_ALGORITHM_MSZIP, NULL, &h))
    {
        free(tmp);
        return -1;
    }

    ok = Decompress(h, tmp, (SIZE_T)tmp_len, dst, (SIZE_T)*dst_len, &out_len);
    CloseDecompressor(h);
    free(tmp);

    if (!ok)
    {
        return -1;
    }

    *dst_len = (size_t)out_len;
    return 0;
}

#endif /* PLATFORM_WINDOWS */
