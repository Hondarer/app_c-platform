/**
 *******************************************************************************
 *  @file           trace_file_internal.h
 *  @brief          ファイルプロバイダ内部管理関数のヘッダーファイル。
 *  @author         Tetsuo Honda
 *  @date           2026/04/03
 *  @version        1.0.0
 *
 *  trace_file.c が公開する内部関数を宣言します。
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

#ifndef TRACE_FILE_PROVIDER_INTERNAL_H
#define TRACE_FILE_PROVIDER_INTERNAL_H

#include <com_util/trace/trace_file.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
 *  @brief          shutdown フェーズでファイルプロバイダハンドルを解放します。
 *  @param[in]      handle 解放するファイルプロバイダハンドル。
 *
 *  内部ミューテックスを取得せずにハンドルを解放します。
 *  呼び出し時点で com_util_trace_file_sink_write() を実行中のスレッドが存在する場合は
 *  未定義動作になります。
 *  通常は trace_registry_dispose_all_on_shutdown() 経由で呼ばれるため、
 *  呼び出し側がスレッドの静止を保証します。
 */
    void com_util_trace_file_sink_dispose_on_shutdown(com_util_trace_file_sink *handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* TRACE_FILE_PROVIDER_INTERNAL_H */
