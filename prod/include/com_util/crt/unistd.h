/**
 *******************************************************************************
 *  @file           unistd.h
 *  @brief          unistd/io 系 CRT 抽象 API です。
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

#ifndef COM_UTIL_CRT_UNISTD_H
#define COM_UTIL_CRT_UNISTD_H

#include <stdarg.h>
#include <com_util/base/compiler.h>
#include <com_util/base/platform.h>
#include <com_util/com_util_export.h>

#if defined(PLATFORM_LINUX)
    #include <unistd.h>
#elif defined(PLATFORM_WINDOWS)
    #include <io.h>
#endif /* PLATFORM_ */

/**
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @{
 */

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

/**
 *  @brief          標準ストリームの種別を表す列挙型です。
 *
 *  com_util_isatty() の引数に使います。
 */
typedef enum com_util_stream
{
    COM_UTIL_STREAM_STDIN = 0,  /**< 標準入力 (stdin)。 */
    COM_UTIL_STREAM_STDOUT = 1, /**< 標準出力 (stdout)。 */
    COM_UTIL_STREAM_STDERR = 2  /**< 標準エラー出力 (stderr)。 */
} com_util_stream_t;

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          指定したストリームが端末 (コンソール/TTY) に接続されているかを判定します。
     *  @param[in]      stream  判定するストリーム (@ref COM_UTIL_STREAM_STDIN 等)。
     *  @return         端末に接続されている場合は 1、それ以外 (リダイレクト、パイプ、
     *                  不正な @p stream 値) は 0 を返します。
     *
     *  Windows 環境では @c GetFileType が @c FILE_TYPE_CHAR を返し、
     *  かつ @c GetConsoleMode が成功する場合にのみ 1 を返します。\n
     *  Linux 環境では POSIX の @c isatty() を使用します。\n
     *  POSIX の @c isatty() と異なり、引数はファイル ディスクリプタではなく
     *  ストリーム enum (@ref com_util_stream_t) です。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_isatty(com_util_stream_t stream);

    /**
     *  @brief          UTF-8 パスのアクセス確認 (`access` / `_waccess` ラッパー) です。
     *  @param[in]      path  確認対象のファイル パス (UTF-8)。NULL を渡してはなりません。
     *  @param[in]      mode  確認するアクセス種別 (@ref COM_UTIL_ACCESS_FMT_F_OK 等)。
     *  @return         アクセス可能時は 0、不可時は -1 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_access(const char *path, int mode);

    /**
     *  @brief          書式指定パスのアクセス確認です。
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
     *  @brief          書式指定パスのアクセス確認 (`com_util_access_fmt` の `va_list` 版) です。
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

/** @} */

#endif /* COM_UTIL_CRT_UNISTD_H */
