/**
 *******************************************************************************
 *  @file           fcntl.h
 *  @brief          fcntl 系 CRT 抽象 API。
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

#ifndef COM_UTIL_CRT_FCNTL_H
#define COM_UTIL_CRT_FCNTL_H

#include <stdarg.h>
#include <fcntl.h>
#include <com_util/base/compiler.h>
#include <com_util/com_util_export.h>

/**
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          UTF-8 パスでファイル記述子を開きます (`open` / `_wopen` ラッパー)。
     *  @param[in]      path   開くファイルのパス (UTF-8)。NULL を渡してはなりません。
     *  @param[in]      flags  オープン フラグ (O_RDONLY、O_WRONLY、O_RDWR など)。
     *  @param[in]      mode   ファイル生成時のパーミッション (O_CREAT 指定時に使用)。
     *  @return         成功時はファイル記述子、失敗時は -1 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_open(const char *path, int flags, int mode);

    /**
     *  @brief          書式指定パスでファイル記述子を開きます。
     *  @param[in]      flags   オープン フラグ (O_RDONLY、O_WRONLY、O_RDWR など)。
     *  @param[in]      mode    ファイル生成時のパーミッション (O_CREAT 指定時に使用)。
     *  @param[in]      format  パスを構築する printf 形式の書式文字列。
     *  @param[in]      ...     書式引数。
     *  @return         成功時はファイル記述子、失敗時は -1 を返します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_open_fmt(int flags, int mode, const char *format, ...)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 3, 4)))
#endif /* COMPILER_GCC */
        ;

    /**
     *  @brief          書式指定パスでファイル記述子を開きます (`com_util_open_fmt` の `va_list` 版)。
     *  @param[in]      flags   オープン フラグ (O_RDONLY、O_WRONLY、O_RDWR など)。
     *  @param[in]      mode    ファイル生成時のパーミッション (O_CREAT 指定時に使用)。
     *  @param[in]      format  パスを構築する printf 形式の書式文字列。
     *  @param[in]      args    書式引数リスト。
     *  @return         成功時はファイル記述子、失敗時は -1 を返します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_vopen_fmt(int flags, int mode, const char *format, va_list args)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 3, 0)))
#endif /* COMPILER_GCC */
        ;

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_CRT_FCNTL_H */
