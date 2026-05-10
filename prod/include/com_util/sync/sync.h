/**
 *******************************************************************************
 *  @file           sync.h
 *  @brief          スレッド・同期プリミティブ抽象レイヤーのヘッダー。
 *  @author         Tetsuo Honda
 *  @date           2026/04/20
 *  @version        2.0.0
 *
 *  @details
 *  OS ごとの差異を隠蔽し、プロセス内同期とプロセス横断可能な
 *  アプリケーション排他を共通の結果コードで提供します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef COM_UTIL_SYNC_H
#define COM_UTIL_SYNC_H

#include <stddef.h>
#include <stdint.h>

#include <com_util_export.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define COM_UTIL_SYNC_WAIT_FOREVER UINT32_MAX
#define COM_UTIL_SYNC_NO_WAIT 0U

typedef enum
{
    COM_UTIL_SYNC_OK = 0,
    COM_UTIL_SYNC_TIMEOUT = 1,
    COM_UTIL_SYNC_BUSY = 2,
    COM_UTIL_SYNC_INVALID_ARGUMENT = 3,
    COM_UTIL_SYNC_UNSUPPORTED = 4,
    COM_UTIL_SYNC_SYSTEM_ERROR = 5,
    COM_UTIL_SYNC_CORRUPT_DESCRIPTOR = 6,
    COM_UTIL_SYNC_BUFFER_TOO_SMALL = 7
} com_util_sync_result_t;

typedef enum
{
    COM_UTIL_APP_LOCK_BACKEND_LOCK_FILE = 1
} com_util_app_lock_backend_t;

typedef struct com_util_mutex com_util_mutex_t;
typedef struct com_util_condvar com_util_condvar_t;
typedef struct com_util_rwlock com_util_rwlock_t;
typedef struct com_util_thread com_util_thread_t;
typedef struct com_util_app_lock com_util_app_lock_t;

/** スレッド関数ポインタ型。 */
typedef void (*com_util_thread_func_t)(void *);
/** call_once で 1 回だけ呼び出される関数ポインタ型。 */
typedef void (*com_util_once_func_t)(void);

/** call_once 状態。静的領域では 0 初期化して用いる。 */
typedef struct
{
    volatile int32_t state;
} com_util_once_flag_t;

COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_mutex_create(com_util_mutex_t **mtx);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_mutex_lock(com_util_mutex_t *mtx, uint32_t timeout_ms);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_mutex_try_lock(com_util_mutex_t *mtx);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_mutex_unlock(com_util_mutex_t *mtx);
COM_UTIL_EXPORT void COM_UTIL_API com_util_mutex_destroy(com_util_mutex_t *mtx);

COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_condvar_create(com_util_condvar_t **cv);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_condvar_wait(com_util_condvar_t *cv, com_util_mutex_t *mtx, uint32_t timeout_ms);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_condvar_signal(com_util_condvar_t *cv);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_condvar_broadcast(com_util_condvar_t *cv);
COM_UTIL_EXPORT void COM_UTIL_API com_util_condvar_destroy(com_util_condvar_t *cv);

COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_rwlock_create(com_util_rwlock_t **rwlock);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_rwlock_lock_shared(com_util_rwlock_t *rwlock, uint32_t timeout_ms);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_rwlock_try_lock_shared(com_util_rwlock_t *rwlock);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_rwlock_lock_exclusive(com_util_rwlock_t *rwlock, uint32_t timeout_ms);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_rwlock_try_lock_exclusive(com_util_rwlock_t *rwlock);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_rwlock_unlock_shared(com_util_rwlock_t *rwlock);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_rwlock_unlock_exclusive(com_util_rwlock_t *rwlock);
COM_UTIL_EXPORT void COM_UTIL_API com_util_rwlock_destroy(com_util_rwlock_t *rwlock);

COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_thread_create(com_util_thread_t **thread, com_util_thread_func_t func, void *arg);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_thread_join(com_util_thread_t *thread, uint32_t timeout_ms);
COM_UTIL_EXPORT void COM_UTIL_API com_util_thread_detach(com_util_thread_t *thread);

COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_app_lock_create(const char *identity, com_util_app_lock_t **lock);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_app_lock_open(const char *identity, com_util_app_lock_t **lock);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_app_lock_import_descriptor(const void *descriptor, size_t descriptor_size,
                                    com_util_app_lock_t **lock);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_app_lock_export_descriptor(const com_util_app_lock_t *lock, void *descriptor,
                                    size_t *descriptor_size);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_app_lock_lock_shared(com_util_app_lock_t *lock, uint32_t timeout_ms);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_app_lock_try_lock_shared(com_util_app_lock_t *lock);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_app_lock_lock_exclusive(com_util_app_lock_t *lock, uint32_t timeout_ms);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_app_lock_try_lock_exclusive(com_util_app_lock_t *lock);
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_app_lock_unlock(com_util_app_lock_t *lock);
COM_UTIL_EXPORT void COM_UTIL_API com_util_app_lock_destroy(com_util_app_lock_t *lock);

COM_UTIL_EXPORT void COM_UTIL_API com_util_call_once(com_util_once_flag_t *flag,
                                                     com_util_once_func_t func);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COM_UTIL_SYNC_H */
