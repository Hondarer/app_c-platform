/**
 *******************************************************************************
 *  @file           shutdown.h
 *  @brief          プロセス終了ハンドリング共通 API。
 *  @author         Tetsuo Honda
 *  @date           2026/05/06
 *
 *  @details
 *  通常終了時の cleanup callback と、補足可能な終了要求の request callback を提供します。\n
 *  いずれも登録したコールバックは LIFO 順で 1 回だけ実行されます。\n
 *  `atexit()` 経路では C 標準の制約により `exit(code)` の引数を直接取得できません。\n
 *  終了コードを確実に渡したい場合は `com_util_exit()` を使用してください。\n
 *  `TerminateProcess`、`_exit`、クラッシュ、強制 kill などの補足不能な終了は保証対象外です。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef COM_UTIL_RUNTIME_SHUTDOWN_H
#define COM_UTIL_RUNTIME_SHUTDOWN_H

#include <com_util/base/platform.h>
#include <com_util_export.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @enum           com_util_shutdown_reason_t
     *  @brief          終了理由の種別。
     */
    typedef enum com_util_shutdown_reason_t
    {
        COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT = 0,            /**< 通常終了。 */
        COM_UTIL_SHUTDOWN_REASON_PROCESS_TERMINATING = 1,    /**< 終了処理中で待機を避けるべき終了。 */
        COM_UTIL_SHUTDOWN_REASON_SIGNAL_OR_CONSOLE_EVENT = 2 /**< シグナルまたはコンソール イベント。 */
    } com_util_shutdown_reason_t;

    /**
     *  @enum           com_util_shutdown_code_kind_t
     *  @brief          終了イベントに付随する数値コードの意味。
     */
    typedef enum com_util_shutdown_code_kind_t
    {
        COM_UTIL_SHUTDOWN_CODE_KIND_NONE = 0,             /**< 追加コードなし。 */
        COM_UTIL_SHUTDOWN_CODE_KIND_EXIT_CODE = 1,        /**< `exit(code)` の終了コード。 */
        COM_UTIL_SHUTDOWN_CODE_KIND_SIGNAL_NUMBER = 2,    /**< `SIGINT` などのシグナル番号。 */
        COM_UTIL_SHUTDOWN_CODE_KIND_CONSOLE_CTRL_TYPE = 3 /**< Windows `CTRL_*_EVENT`。 */
    } com_util_shutdown_code_kind_t;

    /**
     *  @brief          終了イベント情報。
     */
    typedef struct com_util_shutdown_event_t
    {
        com_util_shutdown_reason_t reason;       /**< 終了理由。 */
        com_util_shutdown_code_kind_t code_kind; /**< `code` の意味。 */
        int code;                                /**< 終了コード、シグナル番号、CTRL 種別。 */
    } com_util_shutdown_event_t;

    /**
     *  @brief          終了コールバック関数型。
     *  @param[in]      event   終了イベント情報。
     *  @param[in]      context 登録時に渡した任意のコンテキスト。
     *
     *  @par            スレッド セーフ
     *  コールバックはシャットダウン ハンドラから 1 スレッドで呼び出されます。\n
     *  コールバック内で再帰的にシャットダウン処理を呼び出さないでください。
     */
    typedef void (*com_util_shutdown_callback_t)(const com_util_shutdown_event_t *event, void *context);

    /**
     *  @brief          終了コールバックを登録する。
     *
     *  登録済みコールバックは、通常終了または補足可能な終了イベント時に LIFO 順で
     *  1 回だけ実行されます。\n
     *  shutdown 開始後の登録は失敗します。
     *
     *  @param[in]      callback 実行するコールバック。
     *  @param[in]      context  コールバックへ渡す任意ポインタ。NULL 可。
     *  @return         成功 0 / 失敗 -1。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部の shutdown_lock で保護されており、複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_shutdown_register(com_util_shutdown_callback_t callback, void *context);

    /**
     *  @brief          終了要求 callback を登録する。
     *
     *  `SIGINT` / `SIGTERM` / `CTRL_C_EVENT` など、補足可能な終了要求で
     *  LIFO 順に 1 回だけ実行されます。\n
     *  request callback の実行後も final shutdown callback は未実行のまま保持され、
     *  通常終了時に別途実行されます。\n
     *  shutdown 開始後または終了要求通知後の登録は失敗します。
     *
     *  @param[in]      callback 実行するコールバック。
     *  @param[in]      context  コールバックへ渡す任意ポインタ。NULL 可。
     *  @return         成功 0 / 失敗 -1。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部の shutdown_lock で保護されており、複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_shutdown_request_register(com_util_shutdown_callback_t callback,
                                                                        void *context);

    /**
     *  @brief          終了コードを記録して `exit(code)` を実行する。
     *  @param[in]      code 終了コード。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  プロセス内で 1 スレッドのみが呼び出してください。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_exit(int code);

    /**
     *  @brief          テスト用に任意の終了イベントを同期実行する。
     *
     *  実アプリケーションでは使用しません。登録済みコールバックを 1 回だけ実行します。
     *
     *  @param[in]      event 実行に使用する終了イベント。
     *  @return         0: 実行した / 1: すでに実行済み / -1: 引数不正。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部の shutdown_lock で保護されており、複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API _com_util_shutdown_invoke_for_test(const com_util_shutdown_event_t *event);

    /**
     *  @brief          テスト用に終了要求 callback を同期実行する。
     *
     *  実アプリケーションでは使用しません。登録済み request callback を 1 回だけ実行します。
     *
     *  @param[in]      event 実行に使用する終了イベント。
     *  @return         0: 実行した / 1: すでに実行済み / -1: 引数不正。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部の shutdown_lock で保護されており、複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API _com_util_shutdown_request_invoke_for_test(const com_util_shutdown_event_t *event);

    /**
     *  @brief          テスト用に shutdown ランタイムの内部状態を初期化する。
     *
     *  すでに登録済みの callback は破棄されます。\n
     *  既存モジュール側の `call_once` 状態までは巻き戻しません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部の shutdown_lock で保護されており、複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT void COM_UTIL_API _com_util_shutdown_reset_for_test(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COM_UTIL_RUNTIME_SHUTDOWN_H */
