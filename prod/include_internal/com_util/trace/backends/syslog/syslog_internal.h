/**
 *******************************************************************************
 *  @file           syslog_internal.h
 *  @brief          syslog プロバイダ内部管理関数のヘッダーファイル。
 *  @author         Tetsuo Honda
 *  @date           2026/04/03
 *  @version        1.0.0
 *
 *  trace_syslog.c が公開する内部関数を宣言します。
 *  このヘッダーはモジュール内部でのみ使用します。
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

#ifndef TRACE_SYSLOG_PROVIDER_INTERNAL_H
#define TRACE_SYSLOG_PROVIDER_INTERNAL_H

#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)
    #include <com_util/trace/syslog.h>

    #ifdef __cplusplus
extern "C"
{
    #endif /* __cplusplus */

    /**
 *  @brief          shutdown フェーズで syslog プロバイダハンドルを解放します。
 *  @param[in]      handle 解放する syslog プロバイダハンドル。
 *
 *  reconnect_lock を取得せずにソケットを閉じてハンドルを解放します。
 *  呼び出し時点で com_util_syslog_sink_write() を実行中のスレッドが存在する場合は
 *  未定義動作になります。
 *  通常は trace_registry_dispose_all_on_shutdown() 経由で呼ばれるため、
 *  呼び出し側がスレッドの静止を保証します。
 */
    void com_util_syslog_sink_dispose_on_shutdown(com_util_syslog_sink *handle);

    #ifdef __cplusplus
}
    #endif /* __cplusplus */

#endif /* PLATFORM_LINUX */

#endif /* TRACE_SYSLOG_PROVIDER_INTERNAL_H */
