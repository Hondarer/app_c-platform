/**
 *******************************************************************************
 *  @file           etw_internal.h
 *  @brief          ETW プロバイダ内部管理関数のヘッダーファイル。
 *  @author         Tetsuo Honda
 *  @date           2026/04/03
 *  @version        1.0.0
 *
 *  trace_etw.c が公開する内部関数を宣言します。
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

#ifndef TRACE_ETW_PROVIDER_INTERNAL_H
#define TRACE_ETW_PROVIDER_INTERNAL_H

#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)
    #include <com_util/trace/etw.h>
    #include <com_util/runtime/shutdown.h>

    #ifdef __cplusplus
extern "C"
{
    #endif /* __cplusplus */

    /**
 *  @brief          shutdown フェーズで ETW プロバイダハンドルを解放します。
 *  @param[in]      handle 解放する ETW プロバイダハンドル。
 *  @param[in]      event shutdown イベント情報。
 *
 *  `process_terminating` 相当のイベントでは free() のみ実行します。\n
 *  通常終了では TraceLoggingUnregister() を呼び出します。
 *  呼び出し時点で com_util_etw_provider_write() を実行中のスレッドが存在する場合は
 *  未定義動作になります。
 *  通常は trace_registry_dispose_all_on_shutdown() 経由で呼ばれるため、
 *  呼び出し側がスレッドの静止を保証します。
 */
    void com_util_etw_provider_dispose_on_shutdown(com_util_etw_provider *handle, const com_util_shutdown_event *event);

    #ifdef __cplusplus
}
    #endif /* __cplusplus */

#endif /* PLATFORM_WINDOWS */

#endif /* TRACE_ETW_PROVIDER_INTERNAL_H */
