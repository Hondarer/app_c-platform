/**
 *******************************************************************************
 *  @file           process_internal.h
 *  @brief          プロセス起動ユーティリティの内部共有ヘッダーです。
 *  @author         Tetsuo Honda
 *  @date           2026/06/20
 *
 *  process.c が公開する、ライブラリ内部向けの拡張関数を宣言します。
 *  このヘッダーはモジュール内部でのみ使用します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_RUNTIME_PROCESS_INTERNAL_H
#define COM_UTIL_RUNTIME_PROCESS_INTERNAL_H

#include <stdint.h>
#include <com_util/runtime/process.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          既存の OS プロセス ハンドルを com_util_process でラップします。
     *  @param[in]      native_handle  ラップする OS ネイティブ ハンドル。
     *                                 Linux ではプロセス ID (pid_t)、Windows では HANDLE。
     *  @return         成功時はプロセス ハンドル、失敗時は NULL を返します。
     *
     *  自前で子プロセスを起動せず外部で得たプロセス ハンドル (例: ShellExecuteExW の
     *  hProcess) を、com_util_process_wait() などの共通 API で扱えるようにします。\n
     *  返したハンドルの所有権は呼び出し側へ移り、com_util_process_destroy() で解放します。
     *  Windows では destroy 時に CloseHandle() が呼ばれます。\n
     *  本関数はプロセス起動ユーティリティ内部の拡張であり、昇格などの上位コンポーネント
     *  から共通の待機・終了コード取得・破棄を再利用するために用います。
     */
    com_util_process *com_util_process_adopt_native(intptr_t native_handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COM_UTIL_RUNTIME_PROCESS_INTERNAL_H */
