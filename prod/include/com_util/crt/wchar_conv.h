/**
 *******************************************************************************
 *  @file           wchar_conv.h
 *  @brief          Windows で UTF-8 とワイド文字列を相互変換する API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/06/09
 *  @version        1.0.0
 *
 *  Windows 上で UTF-8 文字列とワイド文字列 (UTF-16LE) を相互変換する
 *  関数を提供します。\n
 *  Linux では本ヘッダーは何も宣言しません。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef COM_UTIL_CRT_WCHAR_CONV_H
#define COM_UTIL_CRT_WCHAR_CONV_H

/**
 *  @ingroup        COM_UTIL_CRT
 *  @{
 */

#include <com_util/com_util_export.h>

#if defined(PLATFORM_WINDOWS)

    #include <stddef.h>
    #include <wchar.h>

/**
 *  @brief          UTF-8 パス文字列をワイド文字列に変換します。
 *  @param[out]     wbuf        変換結果の書き込み先バッファー。
 *  @param[in]      wbuf_count  wbuf の要素数。
 *  @param[in]      utf8_path   変換元の UTF-8 パス文字列。
 *  @return         変換後の文字数 (null 終端含む)。失敗時は -1。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_utf8_to_wpath(wchar_t *wbuf, size_t wbuf_count, const char *utf8_path);

/**
 *  @brief          ワイド文字列パスを UTF-8 に変換します。
 *
 *  変換後の文字列中の `\\` を `/` に正規化します。
 *  @param[out]     out         変換結果の書き込み先バッファー。
 *  @param[in]      out_size    out のバイト数。
 *  @param[in]      wpath       変換元のワイド文字列パス。
 *  @return         変換後のバイト数 (null 終端含む)。失敗時は -1。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_wpath_to_utf8(char *out, size_t out_size, const wchar_t *wpath);

/**
 *  @brief          UTF-8 文字列をワイド文字列に変換し、malloc() で確保して返します。
 *  @param[in]      utf8_text   変換元の UTF-8 文字列。
 *  @return         呼び出し元が free() すべきワイド文字列。失敗時は NULL。
 */
COM_UTIL_EXPORT wchar_t *COM_UTIL_API com_util_utf8_to_wstr_alloc(const char *utf8_text);

/**
 *  @brief          ワイド文字列を UTF-8 に変換し、malloc() で確保して返します。
 *  @param[in]      wtext       変換元のワイド文字列。
 *  @return         呼び出し元が free() すべき UTF-8 文字列。失敗時は NULL。
 */
COM_UTIL_EXPORT char *COM_UTIL_API com_util_wstr_to_utf8_alloc(const wchar_t *wtext);

#endif /* PLATFORM_WINDOWS */

/** @} */

#endif /* COM_UTIL_CRT_WCHAR_CONV_H */
