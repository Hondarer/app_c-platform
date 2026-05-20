/**
 *******************************************************************************
 *  @file           compress_linux.c
 *  @brief          Linux 向け圧縮・展開モジュール (zlib)。
 *  @author         Tetsuo Honda
 *  @date           2026/03/05
 *  @version        1.0.0
 *
 *  zlib の deflate/inflate を raw DEFLATE (windowBits = -15) モードで使用します。\n
 *  Windows 実装 (MSZIP | COMPRESS_RAW) と同一フォーマットを出力するため、
 *  クロスプラットフォーム通信に対応します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)

    #include <string.h>

    #include <arpa/inet.h>
    #include <zlib.h>

    #include <com_util/compress/compress.h>

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_compress(uint8_t *dst, size_t *dst_len, const uint8_t *src, const size_t src_len)
{
    uint32_t orig_len_nbo;
    z_stream z;
    int ret;

    if (dst == NULL || dst_len == NULL || src == NULL || src_len == 0)
    {
        return -1;
    }

    if (*dst_len < COM_UTIL_COMPRESS_HEADER_SIZE + 1U)
    {
        return -1;
    }

    /* 先頭 4 バイトに元サイズ (NBO) を書く */
    orig_len_nbo = htonl((uint32_t)src_len);
    memcpy(dst, &orig_len_nbo, COM_UTIL_COMPRESS_HEADER_SIZE);

    /* raw DEFLATE (windowBits = -15) で圧縮 */
    memset(&z, 0, sizeof(z));
    z.next_in = (Bytef *)(uintptr_t)src;
    z.avail_in = (uInt)src_len;
    z.next_out = dst + COM_UTIL_COMPRESS_HEADER_SIZE;
    z.avail_out = (uInt)(*dst_len - COM_UTIL_COMPRESS_HEADER_SIZE);

    if (deflateInit2(&z, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) != Z_OK)
    {
        return -1;
    }

    ret = deflate(&z, Z_FINISH);
    deflateEnd(&z);

    if (ret != Z_STREAM_END)
    {
        return -1;
    }

    *dst_len = COM_UTIL_COMPRESS_HEADER_SIZE + (size_t)z.total_out;
    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_decompress(uint8_t *dst, size_t *dst_len, const uint8_t *src, const size_t src_len)
{
    uint32_t orig_len_nbo;
    uint32_t orig_len;
    z_stream z;
    int ret;

    if (dst == NULL || dst_len == NULL || src == NULL || src_len <= COM_UTIL_COMPRESS_HEADER_SIZE)
    {
        return -1;
    }

    /* 先頭 4 バイトから元サイズを取得 */
    memcpy(&orig_len_nbo, src, COM_UTIL_COMPRESS_HEADER_SIZE);
    orig_len = ntohl(orig_len_nbo);

    if (*dst_len < (size_t)orig_len)
    {
        return -1;
    }

    /* raw DEFLATE を展開 */
    memset(&z, 0, sizeof(z));
    z.next_in = (Bytef *)(uintptr_t)(src + COM_UTIL_COMPRESS_HEADER_SIZE);
    z.avail_in = (uInt)(src_len - COM_UTIL_COMPRESS_HEADER_SIZE);
    z.next_out = (Bytef *)dst;
    z.avail_out = (uInt)*dst_len;

    if (inflateInit2(&z, -15) != Z_OK)
    {
        return -1;
    }

    ret = inflate(&z, Z_FINISH);
    inflateEnd(&z);

    if (ret != Z_STREAM_END)
    {
        return -1;
    }

    *dst_len = (size_t)z.total_out;
    return 0;
}

#elif defined(PLATFORM_WINDOWS) && defined(COMPILER_MSVC)
    #pragma warning(disable : 4206)
#endif
