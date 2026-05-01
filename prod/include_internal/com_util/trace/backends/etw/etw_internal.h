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
 *******************************************************************************
 */

#ifndef TRACE_ETW_PROVIDER_INTERNAL_H
#define TRACE_ETW_PROVIDER_INTERNAL_H

#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)
    #include <com_util/trace/etw.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 *******************************************************************************
 *  @brief          ライブラリアンロード時に ETW プロバイダハンドルを解放します。
 *  @param[in]      handle 解放する ETW プロバイダハンドル。
 *  @param[in]      process_terminating プロセス終了による呼び出しの場合は 1、
 *                  明示的なアンロードの場合は 0 を指定します。
 *
 *  @par            DLL ロード/アンロードコンテキスト
 *  本関数は DllMain から呼び出し可能です。\n
 *  **process_terminating=1**: free() のみ実行するため常に安全です。\n
 *  **process_terminating=0**: TraceLoggingUnregister() を呼び出します。
 *  呼び出し時点で com_util_etw_provider_write() を実行中のスレッドが存在する場合は
 *  未定義動作になります。
 *  通常は trace_registry_dispose_all_on_unload() 経由で呼ばれるため、
 *  呼び出し側がスレッドの静止を保証します。
 *******************************************************************************
 */
void com_util_etw_provider_dispose_on_unload(com_util_etw_provider_t *handle, int process_terminating);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PLATFORM_WINDOWS */

#endif /* TRACE_ETW_PROVIDER_INTERNAL_H */
