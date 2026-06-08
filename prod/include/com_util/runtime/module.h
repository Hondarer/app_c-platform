/**
 *******************************************************************************
 *  @file           module.h
 *  @brief          モジュール情報取得 API。
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

#ifndef COM_UTIL_MODULE_H
#define COM_UTIL_MODULE_H

#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
    #endif /* _GNU_SOURCE */
#endif     /* PLATFORM_LINUX */

#include <stddef.h>
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
     *  @brief          関数アドレスが属するモジュールの完全なパスを取得します。
     *  @param[out]     out_path      完全なパス (UTF-8) の格納先。NULL を渡してはなりません。
     *  @param[in]      out_path_sz   @p out_path のサイズ (バイト)。0 を渡してはなりません。
     *  @param[in]      func_addr     モジュールを特定するための関数アドレス。NULL を渡してはなりません。
     *  @return         成功時は 0、失敗時は -1 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_module_get_path(char *out_path, size_t out_path_sz,
                                                              const void *func_addr);

    /**
     *  @brief          関数アドレスが属するモジュールのベース名を取得します。
     *  @param[out]     out_basename      ベース名 (UTF-8) の格納先。NULL を渡してはなりません。
     *  @param[in]      out_basename_sz   @p out_basename のサイズ (バイト)。0 を渡してはなりません。
     *  @param[in]      func_addr         モジュールを特定するための関数アドレス。NULL を渡してはなりません。
     *  @return         成功時は 0、失敗時は -1 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_module_get_basename(char *out_basename, size_t out_basename_sz,
                                                                  const void *func_addr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_MODULE_H */
