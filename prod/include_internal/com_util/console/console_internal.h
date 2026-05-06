/**
 *******************************************************************************
 *  @file           console_internal.h
 *  @brief          コンソールヘルパー内部関数のヘッダーファイル。
 *  @author         Tetsuo Honda
 *  @date           2026/04/04
 *  @version        1.0.0
 *
 *  console.c が公開する内部関数を宣言します。
 *  このヘッダーはモジュール内部でのみ使用します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef CONSOLE_UTIL_INTERNAL_H
#define CONSOLE_UTIL_INTERNAL_H

#include <com_util/runtime/shutdown.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 *******************************************************************************
 *  @brief          shutdown フェーズでコンソールヘルパーを解放します。
 *  @param[in]      event   shutdown イベント情報。
 *  @param[in]      context 登録時に渡した任意のコンテキスト (未使用)。
 *
 *  @details        通常終了ではストリームを元に戻してスレッド終了を待機します。\n
 *                  シグナルや強制終了に近いイベントでは待機を避け、安全側で短絡します。
 *******************************************************************************
 */
void com_util_console_dispose_on_shutdown(const com_util_shutdown_event_t *event, void *context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CONSOLE_UTIL_INTERNAL_H */
