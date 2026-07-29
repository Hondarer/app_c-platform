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
     *  @return         成功時は 0、バッファー不足時は ERANGE を返します。
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
     *  @return         成功時は 0、バッファー不足時は ERANGE を返します。
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
     *  @return         成功時は 0、バッファー不足時は ERANGE を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一の @p dest を複数スレッドから同時に書き換えないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_strcat(char *dest, size_t dest_size, const char *src);

    /**
     *  @brief          バッファー サイズ付き安全 wcscpy (`wcscpy_s` 相当) です。
     *  @param[out]     dest       コピー先バッファー。NULL を渡してはなりません。
     *  @param[in]      dest_size  @p dest のサイズ (wchar_t 単位)。0 を渡してはなりません。
     *  @param[in]      src        コピー元ワイド文字列。NULL を渡してはなりません。
     *  @return         成功時は 0、バッファー不足時は ERANGE を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一の @p dest を複数スレッドから同時に書き換えないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_wcscpy(wchar_t *dest, size_t dest_size, const wchar_t *src);

    /**
     *  @brief          `sscanf` / `sscanf_s` のラッパーです。
     *  @param[in]      buffer  スキャン対象の文字列。NULL を渡してはなりません。
     *  @param[in]      format  scanf 形式の書式文字列。NULL を渡してはなりません。
     *  @param[out]     ...     変換結果の格納先。
     *  @return         成功時は変換した項目数、失敗または EOF 時は EOF を返します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_sscanf(const char *buffer, const char *format, ...)
#if defined(COMPILER_GCC)
        __attribute__((format(scanf, 2, 3)))
#endif /* COMPILER_GCC */
        ;

    /**
     *  @brief          `com_util_sscanf` の `va_list` 版です。
     *  @param[in]      buffer  スキャン対象の文字列。NULL を渡してはなりません。
     *  @param[in]      format  scanf 形式の書式文字列。NULL を渡してはなりません。
     *  @param[in]      args    書式引数リスト。
     *  @return         成功時は変換した項目数、失敗または EOF 時は EOF を返します。
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
