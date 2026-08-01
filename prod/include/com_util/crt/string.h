/**
 *******************************************************************************
 *  @file           string.h
 *  @brief          string 系の CRT 関数を抽象化する API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/04/22
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_CRT_STRING_H
#define COM_UTIL_CRT_STRING_H

#include <stddef.h>
#include <stdarg.h>
#include <wchar.h>
#include <com_util/base/compiler.h>
#include <com_util/base/result.h>
#include <com_util/com_util_export.h>

/**
 *  @ingroup        COM_UTIL_CRT
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          バッファー サイズ付き安全 strcpy (`strcpy_s` / `strlcpy` 相当) です。
     *  @param[out]     dest       コピー先バッファー。NULL を渡してはなりません。
     *  @param[in]      dest_size  @p dest のサイズ (バイト)。0 を渡してはなりません。
     *  @param[in]      src        コピー元文字列。NULL を渡してはなりません。
     *  @return         成功時は @ref COM_UTIL_OK 、引数不正時は @ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  バッファー不足時は @ref COM_UTIL_ERR_BUFFER_TOO_SMALL を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一の @p dest を複数スレッドから同時に書き換えないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_strcpy(char *dest, size_t dest_size, const char *src);

    /**
     *  @brief          バッファー サイズ付き安全 strncpy (`strncpy_s` 相当) です。
     *  @param[out]     dest       コピー先バッファー。NULL を渡してはなりません。
     *  @param[in]      dest_size  @p dest のサイズ (バイト)。0 を渡してはなりません。
     *  @param[in]      src        コピー元文字列。NULL を渡してはなりません。
     *  @param[in]      count      コピーする最大文字数。
     *  @return         成功時は @ref COM_UTIL_OK 、引数不正時は @ref COM_UTIL_ERR_INVALID_ARGUMENT を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一の @p dest を複数スレッドから同時に書き換えないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_strncpy(char *dest, size_t dest_size, const char *src, size_t count);

    /**
     *  @brief          バッファー サイズ付き安全 strcat (`strcat_s` / `strlcat` 相当) です。
     *  @param[in,out]  dest       連結先バッファー。NULL を渡してはなりません。
     *  @param[in]      dest_size  @p dest のサイズ (バイト)。0 を渡してはなりません。
     *  @param[in]      src        連結する文字列。NULL を渡してはなりません。
     *  @return         成功時は @ref COM_UTIL_OK 、引数不正時は @ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  バッファー不足時は @ref COM_UTIL_ERR_BUFFER_TOO_SMALL を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一の @p dest を複数スレッドから同時に書き換えないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_strcat(char *dest, size_t dest_size, const char *src);

    /**
     *  @brief          文字列を複製します (`strdup` / `_strdup` ラッパー)。
     *  @param[in]      src  複製元の文字列 (null 終端)。NULL を渡した場合は NULL を返します。
     *  @return         複製した文字列へのポインター。確保に失敗した場合は NULL を返します。
     *
     *  MSVC では `strdup` が非推奨 (C4996) であり `_strdup` が正の名前です。\n
     *  本関数はその名前の差異を吸収します。
     *
     *  返却した領域は呼び出し側が `free` で解放してください。
     *
     *  @attention      本関数は @ref COM_UTIL_OK 系の戻り値規約の適用対象外です。
     *                  複製した領域へのポインターを返し、失敗を NULL で表します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT char *COM_UTIL_API com_util_strdup(const char *src);

    /**
     *  @brief          バッファー サイズ付き安全 wcscpy (`wcscpy_s` 相当) です。
     *  @param[out]     dest       コピー先バッファー。NULL を渡してはなりません。
     *  @param[in]      dest_size  @p dest のサイズ (wchar_t 単位)。0 を渡してはなりません。
     *  @param[in]      src        コピー元ワイド文字列。NULL を渡してはなりません。
     *  @return         成功時は @ref COM_UTIL_OK 、引数不正時は @ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  バッファー不足時は @ref COM_UTIL_ERR_BUFFER_TOO_SMALL を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一の @p dest を複数スレッドから同時に書き換えないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_wcscpy(wchar_t *dest, size_t dest_size, const wchar_t *src);

    /**
     *  @brief          文字列から書式化データを読み取ります (`sscanf` ラッパー)。
     *  @param[in]      buffer  スキャン対象の文字列。NULL を渡してはなりません。
     *  @param[in]      format  scanf 形式の書式文字列。NULL を渡してはなりません。
     *  @param[out]     ...     変換結果の格納先。
     *  @return         成功時は変換した項目数、失敗または EOF 時は EOF を返します。
     *
     *  `%s`、`%S`、`%[` で文字列を格納するときは、必ず宛先バッファー容量より小さい幅を指定してください。
     *  `%c`、`%C` は終端文字を追加しないため、指定幅以上の要素数を持つ宛先を渡してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_sscanf(const char *buffer, const char *format, ...)
#if defined(COMPILER_GCC)
        __attribute__((format(scanf, 2, 3)))
#endif /* COMPILER_GCC */
        ;

    /**
     *  @brief          文字列から書式化データを読み取ります (`com_util_sscanf` の `va_list` 版)。
     *  @param[in]      buffer  スキャン対象の文字列。NULL を渡してはなりません。
     *  @param[in]      format  scanf 形式の書式文字列。NULL を渡してはなりません。
     *  @param[in]      args    書式引数リスト。
     *  @return         成功時は変換した項目数、失敗または EOF 時は EOF を返します。
     *
     *  文字列とスキャンセットの変換には @ref com_util_sscanf と同じ幅指定規約が適用されます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_vsscanf(const char *buffer, const char *format, va_list args)
#if defined(COMPILER_GCC)
        __attribute__((format(scanf, 2, 0)))
#endif /* COMPILER_GCC */
        ;

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_CRT_STRING_H */
