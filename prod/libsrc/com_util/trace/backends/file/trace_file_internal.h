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
 *******************************************************************************
 */

#ifndef TRACE_FILE_PROVIDER_INTERNAL_H
#define TRACE_FILE_PROVIDER_INTERNAL_H

#include <com_util/trace/trace_file.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 *******************************************************************************
 *  @brief          ライブラリアンロード時にファイルプロバイダハンドルを解放します。
 *  @param[in]      handle 解放するファイルプロバイダハンドル。
 *
 *  @par            DLL ロード/アンロードコンテキスト
 *  本関数は DllMain および constructor/destructor から呼び出し可能です。\n
 *  内部ミューテックスを取得せずにハンドルを解放します。
 *  呼び出し時点で com_util_trace_file_sink_write() を実行中のスレッドが存在する場合は
 *  未定義動作になります。
 *  通常は trace_registry_dispose_all_on_unload() 経由で呼ばれるため、
 *  呼び出し側がスレッドの静止を保証します。
 *******************************************************************************
 */
void com_util_trace_file_sink_dispose_on_unload(com_util_trace_file_sink_t *handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* TRACE_FILE_PROVIDER_INTERNAL_H */
