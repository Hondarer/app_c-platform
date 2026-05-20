/**
 *******************************************************************************
 *  @file           tracer_internal.h
 *  @brief          トレースプロバイダ内部管理関数のヘッダーファイル。
 *  @author         Tetsuo Honda
 *  @date           2026/04/03
 *  @version        1.0.0
 *
 *  tracer.c が公開する内部関数を宣言します。
 *  このヘッダーはモジュール内部でのみ使用します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef COM_UTIL_TRACER_INTERNAL_H
#define COM_UTIL_TRACER_INTERNAL_H

#include <stddef.h>
#include <com_util/runtime/shutdown.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 *  @brief          shutdown フェーズで全トレースハンドルを解放します。
 *  @param[in]      event shutdown イベント情報。
 *
 *  本関数は終了フェーズ向けの安全側解放を行います。\n
 *  内部でレジストリロックを取得しません。
 *  呼び出し前に、すべてのスレッドが trace API
 *  (com_util_tracer_create / com_util_tracer_dispose / com_util_tracer_write 等) の
 *  呼び出しを完了している必要があります。
 *  並行してトレース API が呼ばれた場合は未定義動作になります。
 */
void trace_registry_dispose_all_on_shutdown(const com_util_shutdown_event_t *event);

/**
 *  @brief          現在アクティブなトレースハンドルの数を返します。
 *  @return         アクティブなハンドルの数。
 */
size_t trace_registry_count(void);

/**
 *  @brief          トレースハンドルレジストリの現在の容量を返します。
 *  @return         レジストリの容量 (確保済みのスロット数)。
 */
size_t trace_registry_capacity(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COM_UTIL_TRACER_INTERNAL_H */
