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

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 *******************************************************************************
 *  @brief          ライブラリアンロード時にコンソールヘルパーを解放します。
 *  @param[in]      process_terminating プロセス終了による呼び出しの場合は 1、
 *                  明示的なアンロードの場合は 0 を指定します。
 *
 *  @par            DLL ロード/アンロードコンテキスト
 *  本関数は DllMain および constructor/destructor から呼び出し可能です。\n
 *  **Linux**: no-op のため常に安全です。\n
 *  **Windows / process_terminating=1**: 即座に返るため安全です。\n
 *  **Windows / process_terminating=0**: 内部で com_util_thread_join_timed (最大 500ms) を
 *  呼び出してリーダースレッドの終了を待ちます。
 *  リーダースレッドはローダーロックを取得する操作を行わないため
 *  デッドロックは発生しません。
 *******************************************************************************
 */
void com_util_console_dispose_on_unload(int process_terminating);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CONSOLE_UTIL_INTERNAL_H */
