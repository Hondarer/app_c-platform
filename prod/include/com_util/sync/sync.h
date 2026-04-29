/**
 *******************************************************************************
 *  @file           sync.h
 *  @brief          スレッド・同期プリミティブ抽象レイヤーのヘッダー。
 *  @author         Tetsuo Honda
 *  @date           2026/04/20
 *  @version        1.0.0
 *
 *  @details
 *  OS ごとに異なるミューテックス・条件変数・リードライトロック・スレッド API を
 *  共通インターフェースで抽象化します。\n
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef COM_UTIL_SYNC_H
#define COM_UTIL_SYNC_H

#include <stdint.h>

#include <com_util/base/platform.h>
#include <com_util_export.h>

/* ============================================================
 * 型定義
 * ============================================================ */
#if defined(PLATFORM_LINUX)
    #include <pthread.h>
    /** ミューテックス型。 */
    typedef pthread_mutex_t com_util_mutex_t;
    /** 条件変数型。 */
    typedef pthread_cond_t com_util_condvar_t;
    /** リードライトロック型。 */
    typedef pthread_rwlock_t com_util_rwlock_t;
    /** スレッドハンドル型。 */
    typedef pthread_t com_util_thread_t;
#elif defined(PLATFORM_WINDOWS)
    #include <com_util/base/windows_sdk.h>
    /** ミューテックス型。 */
    typedef CRITICAL_SECTION com_util_mutex_t;
    /** 条件変数型。 */
    typedef CONDITION_VARIABLE com_util_condvar_t;
    /** リードライトロック型。 */
    typedef SRWLOCK com_util_rwlock_t;
    /** スレッドハンドル型。 */
    typedef HANDLE com_util_thread_t;
#endif /* PLATFORM_ */

/** スレッド関数ポインタ型。 */
typedef void (*com_util_thread_func_t)(void *);

/** call_once 状態。静的領域では 0 初期化して用いる。 */
typedef struct
{
    volatile int32_t state;
} com_util_once_flag_t;

/** call_once で 1 回だけ呼び出される関数ポインタ型。 */
typedef void (*com_util_once_func_t)(void);

/* ============================================================
 * extern 関数宣言 (.c で実装)
 * ============================================================ */
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 *  @brief  ミューテックスを初期化する。
 *  @param[out]  mtx  初期化対象。
 *  @return  0: 成功、非 0: 失敗。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_mutex_init(com_util_mutex_t *mtx);

/**
 *  @brief  ミューテックスをロックする。
 *  @param[in,out]  mtx  取得対象。
 *  @return  0: 成功、非 0: 失敗。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_mutex_lock(com_util_mutex_t *mtx);

/**
 *  @brief  タイムアウト付きでミューテックスを取得する。
 *  @param[in,out]  mtx         取得対象。
 *  @param[in]      timeout_ms  タイムアウト (ミリ秒)。
 *  @return  0: 成功、非 0: 失敗またはタイムアウト。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_mutex_timedlock(com_util_mutex_t *mtx,
                                                          uint32_t          timeout_ms);

/**
 *  @brief  ミューテックスをアンロックする。
 *  @param[in,out]  mtx  解放対象。
 *  @return  0: 成功、非 0: 失敗。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_mutex_unlock(com_util_mutex_t *mtx);

/**
 *  @brief  ミューテックスを破棄する。
 *  @param[in,out]  mtx  破棄対象。
 *  @return  0: 成功、非 0: 失敗。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_mutex_destroy(com_util_mutex_t *mtx);

/**
 *  @brief  条件変数を初期化する。
 *  @param[out]  cv  初期化対象。
 *  @return  0: 成功、非 0: 失敗。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_condvar_init(com_util_condvar_t *cv);

/**
 *  @brief  条件変数を待機する。
 *          呼び出し前にミューテックスを取得しておく必要がある。
 *  @param[in]  cv   条件変数。
 *  @param[in]  mtx  保護ミューテックス (取得済みであること)。
 *  @return  0: 成功、非 0: 失敗。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_condvar_wait(com_util_condvar_t *cv,
                                                       com_util_mutex_t   *mtx);

/**
 *  @brief  タイムアウト付き条件変数待機。
 *          呼び出し前にミューテックスを取得しておく必要がある。
 *  @param[in]  cv         条件変数。
 *  @param[in]  mtx        保護ミューテックス (取得済みであること)。
 *  @param[in]  timeout_ms タイムアウト (ミリ秒)。
 *  @return     0 (シグナル受信またはタイムアウト)。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_condvar_timedwait(com_util_condvar_t *cv,
                                                            com_util_mutex_t   *mtx,
                                                            uint32_t            timeout_ms);

/**
 *  @brief  条件変数に 1 スレッドへシグナルを送る。
 *  @param[in,out]  cv  条件変数。
 *  @return  0: 成功、非 0: 失敗。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_condvar_signal(com_util_condvar_t *cv);

/**
 *  @brief  条件変数に全スレッドへシグナルを送る。
 *  @param[in,out]  cv  条件変数。
 *  @return  0: 成功、非 0: 失敗。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_condvar_broadcast(com_util_condvar_t *cv);

/**
 *  @brief  条件変数を破棄する。
 *  @param[in,out]  cv  破棄対象。
 *  @return  0: 成功、非 0: 失敗。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_condvar_destroy(com_util_condvar_t *cv);

/**
 *  @brief  リードライトロックを初期化する。
 *  @param[out]  rwlock  初期化対象。
 *  @return  0: 成功、非 0: 失敗。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_rwlock_init(com_util_rwlock_t *rwlock);

/**
 *  @brief  リードライトロックの共有ロックを取得する。
 *  @param[in,out]  rwlock  取得対象。
 *  @return  0: 成功、非 0: 失敗。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_rwlock_lock_shared(com_util_rwlock_t *rwlock);

/**
 *  @brief  タイムアウト付きで共有ロックを取得する。
 *  @param[in,out]  rwlock      取得対象。
 *  @param[in]      timeout_ms  タイムアウト (ミリ秒)。
 *  @return  0: 成功、非 0: 失敗またはタイムアウト。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_rwlock_timedlock_shared(com_util_rwlock_t *rwlock,
                                                                  uint32_t           timeout_ms);

/**
 *  @brief  リードライトロックの排他ロックを取得する。
 *  @param[in,out]  rwlock  取得対象。
 *  @return  0: 成功、非 0: 失敗。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_rwlock_lock_exclusive(com_util_rwlock_t *rwlock);

/**
 *  @brief  共有ロックを解放する。
 *  @param[in,out]  rwlock  解放対象。
 *  @return  0: 成功、非 0: 失敗。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_rwlock_unlock_shared(com_util_rwlock_t *rwlock);

/**
 *  @brief  排他ロックを解放する。
 *  @param[in,out]  rwlock  解放対象。
 *  @return  0: 成功、非 0: 失敗。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_rwlock_unlock_exclusive(com_util_rwlock_t *rwlock);

/**
 *  @brief  リードライトロックを破棄する。
 *  @param[in,out]  rwlock  破棄対象。
 *  @return  0: 成功、非 0: 失敗。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_rwlock_destroy(com_util_rwlock_t *rwlock);

/**
 *  @brief  スレッドを生成する。
 *  @param[out]  thread  生成したスレッドハンドルの格納先。
 *  @param[in]   func    スレッド関数。
 *  @param[in]   arg     スレッド関数に渡す引数。
 *  @return  0: 成功、非 0: 失敗。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_thread_create(com_util_thread_t     *thread,
                                                        com_util_thread_func_t func,
                                                        void                  *arg);

/**
 *  @brief  スレッドの終了を待機し、ハンドルを解放する。
 *  @param[in,out]  thread  スレッドハンドルへのポインタ。
 */
COM_UTIL_EXPORT void COM_UTIL_API com_util_thread_join(com_util_thread_t *thread);

/**
 *  @brief  タイムアウト付きでスレッドの終了を待機する。
 *  @param[in,out]  thread      スレッドハンドルへのポインタ。
 *  @param[in]      timeout_ms  タイムアウト (ミリ秒)。
 *  @return  0: 成功、1: タイムアウト、-1: 失敗。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_thread_join_timed(com_util_thread_t *thread,
                                                            uint32_t           timeout_ms);

/**
 *  @brief  スレッドハンドルの待機責務を放棄して解放する。
 *  @param[in,out]  thread  スレッドハンドルへのポインタ。
 *  @return  0: 成功、非 0: 失敗。
 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_thread_detach(com_util_thread_t *thread);

/**
 *  @brief  指定関数を 1 回だけ呼び出す。
 *  @param[in,out]  flag  call_once 状態。静的領域では 0 初期化して用いる。
 *  @param[in]      func  1 回だけ実行する関数。
 */
COM_UTIL_EXPORT void COM_UTIL_API com_util_call_once(com_util_once_flag_t *flag,
                                                     com_util_once_func_t  func);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COM_UTIL_SYNC_H */
