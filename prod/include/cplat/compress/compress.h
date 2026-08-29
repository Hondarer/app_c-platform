/**
 *******************************************************************************
 *  @file           compress.h
 *  @brief          データを圧縮および展開する API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/03/05
 *  @version        1.0.0
 *
 *  プラットフォームごとに異なる圧縮 API を共通インターフェースで抽象化します。\n
 *
 *  | OS      | 使用ライブラリ                                   | フォーマット        |
 *  | ------- | ------------------------------------------------ | ------------------- |
 *  | Linux   | zlib (deflate/inflate, windowBits = -15)         | raw DEFLATE         |
 *  | Windows | Compression API (MSZIP \| COMPRESS_RAW)          | raw DEFLATE         |
 *
 *  両 OS とも raw DEFLATE (RFC 1951) を出力するため、クロスプラットフォーム互換
 *  通信が可能です。
 *
 *  圧縮ペイロードのフォーマット:
    @code
    [元サイズ: uint32_t (ネットワークバイトオーダー)] [raw DEFLATE データ]
    @endcode
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COMPRESS_H
#define COMPRESS_H

#include <cplat/base/result.h>
#include <cplat/cplat_export.h>
#include <stddef.h>
#include <stdint.h>

/**
 *  @ingroup        CPLAT_COMPRESS
 *  @{
 */

/** 圧縮ペイロード先頭に付加する元サイズ フィールドのバイト数。 */
#define CPLAT_COMPRESS_HEADER_SIZE 4U

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          データを圧縮します。
     *  @param[out]     dst      圧縮後データを格納するバッファー。
     *                           先頭 4 バイトに元サイズ (NBO) が書き込まれます。
     *  @param[in,out]  dst_len  入力: dst のバッファー サイズ。
     *                           出力: 書き込んだバイト数。
     *  @param[in]      src      圧縮前データへのポインター。
     *  @param[in]      src_len  圧縮前データのバイト数。
     *  @return         @ref CPLAT_OK 、@ref CPLAT_ERR_INVALID_ARGUMENT 、@ref CPLAT_ERR_BUFFER_TOO_SMALL 、@ref CPLAT_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    CPLAT_EXPORT int CPLAT_API cplat_compress(uint8_t *dst, size_t *dst_len, const uint8_t *src,
                                                       size_t src_len);

    /**
     *  @brief          圧縮データを展開します。
     *  @param[out]     dst      展開後データを格納するバッファー。
     *  @param[in,out]  dst_len  入力: dst のバッファー サイズ。
     *                           出力: 書き込んだバイト数。
     *  @param[in]      src      圧縮後データへのポインター (先頭 4 バイトは元サイズ)。
     *  @param[in]      src_len  圧縮後データのバイト数 (ヘッダーを含む)。
     *  @return         @ref CPLAT_OK 、@ref CPLAT_ERR_INVALID_ARGUMENT 、@ref CPLAT_ERR_BUFFER_TOO_SMALL 、@ref CPLAT_ERR_OUT_OF_MEMORY 、@ref CPLAT_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    CPLAT_EXPORT int CPLAT_API cplat_decompress(uint8_t *dst, size_t *dst_len, const uint8_t *src,
                                                         size_t src_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COMPRESS_H */
