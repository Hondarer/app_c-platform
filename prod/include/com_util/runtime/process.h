/**
 *******************************************************************************
 *  @file           process.h
 *  @brief          プロセス情報取得 API。
 *  @author         Tetsuo Honda
 *  @date           2026/06/07
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_PROCESS_H
#define COM_UTIL_PROCESS_H

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
     *  @brief          現在のプロセスの実行ファイル本体の絶対パスを取得します。
     *  @param[out]     out_path      絶対パス (UTF-8) の格納先。NULL を渡してはなりません。
     *  @param[in]      out_path_sz   @p out_path のサイズ (バイト)。0 を渡してはなりません。
     *  @return         成功時は 0、失敗時は -1 を返します。
     *
     *  com_util_module_get_path() が関数アドレスの所属モジュールを返す (Windows では
     *  DLL を指しうる) のに対し、本関数は常にプロセス本体の実行ファイルのパスを返します。\n
     *  返されるパスは UTF-8 文字列で、パス セパレーターは '/' で統一されます。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_process_get_executable_path(char *out_path, size_t out_path_sz);

    /**
     *  @brief          管理者/root 権限が必要な処理のため、必要に応じて昇格実行します。
     *  @param[in]      arguments  昇格実行時に現在の実行ファイルへ渡す引数文字列。NULL 可。
     *  @param[out]     exit_code  昇格プロセスの終了コード、または 0 の格納先。
     *  @param[out]     handled    昇格プロセスで処理した場合は 0 以外、現プロセスで継続する場合は 0 の格納先。
     *  @return         権限確認または昇格プロセス実行が成功した場合は 0、失敗した場合は -1 を返します。
     *
     *  Windows では、未昇格の場合に UAC を要求して現在の実行ファイルを @p arguments 付きで
     *  再起動し、子プロセスの終了まで待機します。すでに昇格済みの場合は何もしません。\n
     *  Linux では、実効ユーザー ID が root であることを確認します。root でない場合は失敗します。\n
     *  本関数は権限保証のための API であり、Linux で `sudo` などの外部昇格コマンドは実行しません。
     *
     *  @par            スレッド セーフ
     *  本関数は内部に共有状態を持ちません。ただし、Windows で UAC を表示するため、
     *  通常はメイン スレッドまたはユーザー操作に応答するスレッドから呼び出してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_process_run_elevated_if_needed(const char *arguments, int *exit_code,
                                                                             int *handled);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_PROCESS_H */
