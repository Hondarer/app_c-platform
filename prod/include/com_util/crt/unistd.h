/**
 *******************************************************************************
 *  @file           unistd.h
 *  @brief          unistd/io 系 CRT 抽象 API。
 *  @author         Tetsuo Honda
 *  @date           2026/04/22
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef COM_UTIL_CRT_UNISTD_H
#define COM_UTIL_CRT_UNISTD_H

#include <stdarg.h>
#include <com_util/base/compiler.h>
#include <com_util/base/platform.h>
#include <com_util_export.h>

#if defined(PLATFORM_LINUX)
    #include <unistd.h>
#elif defined(PLATFORM_WINDOWS)
    #include <io.h>
#endif /* PLATFORM_ */

#ifdef DOXYGEN
    #define COM_UTIL_ACCESS_FMT_F_OK 0 /**< ファイルの存在確認 (F_OK 相当)。 */
    #define COM_UTIL_ACCESS_FMT_R_OK 4 /**< 読み取り可能確認 (R_OK 相当)。 */
    #define COM_UTIL_ACCESS_FMT_W_OK 2 /**< 書き込み可能確認 (W_OK 相当)。 */
#else                                  /* !DOXYGEN */
    #if defined(PLATFORM_LINUX)
        #define COM_UTIL_ACCESS_FMT_F_OK F_OK
        #define COM_UTIL_ACCESS_FMT_R_OK R_OK
        #define COM_UTIL_ACCESS_FMT_W_OK W_OK
    #elif defined(PLATFORM_WINDOWS)
        #define COM_UTIL_ACCESS_FMT_F_OK 0
        #define COM_UTIL_ACCESS_FMT_R_OK 4
        #define COM_UTIL_ACCESS_FMT_W_OK 2
    #endif /* PLATFORM_ */
#endif     /* DOXYGEN */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          UTF-8 パスのアクセス確認 (`access` / `_waccess` ラッパー)。
     *  @param[in]      path  確認対象のファイルパス (UTF-8)。NULL を渡してはなりません。
     *  @param[in]      mode  確認するアクセス種別 (@ref COM_UTIL_ACCESS_FMT_F_OK 等)。
     *  @return         アクセス可能時は 0、不可時は -1 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_access(const char *path, int mode);

    /**
     *  @brief          書式指定パスのアクセス確認。
     *  @param[in]      mode    確認するアクセス種別 (@ref COM_UTIL_ACCESS_FMT_F_OK 等)。
     *  @param[in]      format  パスを構築する printf 形式の書式文字列。
     *  @param[in]      ...     書式引数。
     *  @return         アクセス可能時は 0、不可時は -1 を返します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_access_fmt(int mode, const char *format, ...)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 2, 3)))
#endif /* COMPILER_GCC */
        ;

    /**
     *  @brief          書式指定パスのアクセス確認 (`com_util_access_fmt` の `va_list` 版)。
     *  @param[in]      mode    確認するアクセス種別 (@ref COM_UTIL_ACCESS_FMT_F_OK 等)。
     *  @param[in]      format  パスを構築する printf 形式の書式文字列。
     *  @param[in]      args    書式引数リスト。
     *  @return         アクセス可能時は 0、不可時は -1 を返します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_vaccess_fmt(int mode, const char *format, va_list args)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 2, 0)))
#endif /* COMPILER_GCC */
        ;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COM_UTIL_CRT_UNISTD_H */
