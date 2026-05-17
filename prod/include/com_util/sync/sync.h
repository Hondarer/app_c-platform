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

#define COM_UTIL_SYNC_WAIT_FOREVER UINT32_MAX /**< タイムアウトなしで待機する (UINT32_MAX)。 */
#define COM_UTIL_SYNC_NO_WAIT 0U              /**< 即時リターン (タイムアウト 0 ms)。 */

/** @brief 同期操作の結果コード。 */
typedef enum
{
    COM_UTIL_SYNC_OK = 0,                 /**< 成功。 */
    COM_UTIL_SYNC_TIMEOUT = 1,            /**< タイムアウト。 */
    COM_UTIL_SYNC_BUSY = 2,               /**< リソースがビジー状態。 */
    COM_UTIL_SYNC_INVALID_ARGUMENT = 3,   /**< 引数が不正。 */
    COM_UTIL_SYNC_UNSUPPORTED = 4,        /**< 操作がサポートされない。 */
    COM_UTIL_SYNC_SYSTEM_ERROR = 5,       /**< OS/システムエラー。 */
    COM_UTIL_SYNC_CORRUPT_DESCRIPTOR = 6, /**< ディスクリプタが破損している。 */
    COM_UTIL_SYNC_BUFFER_TOO_SMALL = 7    /**< バッファが不足している。 */
} com_util_sync_result_t;

/** @brief プロセス横断同期のバックエンド種別。 */
typedef enum
{
    COM_UTIL_INTERPROCESS_SYNC_BACKEND_LOCK_FILE = 1 /**< ロックファイルを使用するバックエンド。 */
} com_util_interprocess_sync_backend_t;

typedef struct com_util_local_lock com_util_local_lock_t;               /**< プロセス内ミューテックス。 */
typedef struct com_util_condvar com_util_condvar_t;                     /**< プロセス内条件変数。 */
typedef struct com_util_local_rwlock com_util_local_rwlock_t;           /**< プロセス内読み書きロック。 */
typedef struct com_util_thread com_util_thread_t;                       /**< スレッドハンドル。 */
typedef struct com_util_interprocess_lock com_util_interprocess_lock_t;           /**< プロセス横断ミューテックス。 */
typedef struct com_util_interprocess_rwlock com_util_interprocess_rwlock_t;       /**< プロセス横断読み書きロック。 */

/** スレッド関数ポインタ型。 */
typedef void (*com_util_thread_func_t)(void *);
/** call_once で 1 回だけ呼び出される関数ポインタ型。 */
typedef void (*com_util_once_func_t)(void);

/** call_once 状態。静的領域では 0 初期化して用いる。 */
typedef struct
{
    volatile int32_t state;
} com_util_once_flag_t;

/**
 *  @brief          プロセス内ミューテックスを生成します。
 *  @param[out]     mtx  生成したミューテックスの格納先。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_SYSTEM_ERROR を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_local_lock_create(com_util_local_lock_t **mtx);
/**
 *  @brief          ミューテックスをロックします。
 *  @param[in,out]  mtx        対象のミューテックス。NULL を渡してはなりません。
 *  @param[in]      timeout_ms タイムアウト (ms)。@ref COM_UTIL_SYNC_WAIT_FOREVER または
 *                             @ref COM_UTIL_SYNC_NO_WAIT も指定可能です。
 *  @return         @ref COM_UTIL_SYNC_OK 、@ref COM_UTIL_SYNC_TIMEOUT 、
 *                  @ref COM_UTIL_SYNC_SYSTEM_ERROR のいずれかを返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_local_lock_lock(com_util_local_lock_t *mtx, uint32_t timeout_ms);
/**
 *  @brief          ミューテックスをノンブロッキングでロック試行します。
 *  @param[in,out]  mtx  対象のミューテックス。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_BUSY を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_local_lock_try_lock(com_util_local_lock_t *mtx);
/**
 *  @brief          ミューテックスをアンロックします。
 *  @param[in,out]  mtx  対象のミューテックス。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_SYSTEM_ERROR を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_local_lock_unlock(com_util_local_lock_t *mtx);
/**
 *  @brief          ミューテックスを破棄します。
 *  @param[in,out]  mtx  破棄するミューテックス。NULL を渡してはなりません。
 */
COM_UTIL_EXPORT void COM_UTIL_API com_util_local_lock_destroy(com_util_local_lock_t *mtx);

/**
 *  @brief          条件変数を生成します。
 *  @param[out]     cv  生成した条件変数の格納先。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_SYSTEM_ERROR を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_condvar_create(com_util_condvar_t **cv);
/**
 *  @brief          条件変数を待機します (@p mtx を atomically アンロック後に待機し、シグナル受信後に再ロック)。
 *  @param[in,out]  cv         対象の条件変数。NULL を渡してはなりません。
 *  @param[in,out]  mtx        待機中にアンロックするミューテックス。NULL を渡してはなりません。
 *  @param[in]      timeout_ms タイムアウト (ms)。@ref COM_UTIL_SYNC_WAIT_FOREVER または
 *                             @ref COM_UTIL_SYNC_NO_WAIT も指定可能です。
 *  @return         @ref COM_UTIL_SYNC_OK 、@ref COM_UTIL_SYNC_TIMEOUT 、
 *                  @ref COM_UTIL_SYNC_SYSTEM_ERROR のいずれかを返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_condvar_wait(com_util_condvar_t *cv, com_util_local_lock_t *mtx, uint32_t timeout_ms);
/**
 *  @brief          待機中のスレッドを 1 つ起床させます。
 *  @param[in,out]  cv  対象の条件変数。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_SYSTEM_ERROR を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_condvar_signal(com_util_condvar_t *cv);
/**
 *  @brief          待機中のすべてのスレッドを起床させます。
 *  @param[in,out]  cv  対象の条件変数。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_SYSTEM_ERROR を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_condvar_broadcast(com_util_condvar_t *cv);
/**
 *  @brief          条件変数を破棄します。
 *  @param[in,out]  cv  破棄する条件変数。NULL を渡してはなりません。
 */
COM_UTIL_EXPORT void COM_UTIL_API com_util_condvar_destroy(com_util_condvar_t *cv);

/**
 *  @brief          プロセス内読み書きロックを生成します。
 *  @param[out]     rwlock  生成した読み書きロックの格納先。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_SYSTEM_ERROR を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_local_rwlock_create(com_util_local_rwlock_t **rwlock);
/**
 *  @brief          共有 (読み取り) ロックを取得します。
 *  @param[in,out]  rwlock     対象の読み書きロック。NULL を渡してはなりません。
 *  @param[in]      timeout_ms タイムアウト (ms)。@ref COM_UTIL_SYNC_WAIT_FOREVER または
 *                             @ref COM_UTIL_SYNC_NO_WAIT も指定可能です。
 *  @return         @ref COM_UTIL_SYNC_OK 、@ref COM_UTIL_SYNC_TIMEOUT 、
 *                  @ref COM_UTIL_SYNC_SYSTEM_ERROR のいずれかを返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_local_rwlock_lock_shared(com_util_local_rwlock_t *rwlock, uint32_t timeout_ms);
/**
 *  @brief          共有 (読み取り) ロックをノンブロッキングで取得試行します。
 *  @param[in,out]  rwlock  対象の読み書きロック。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_BUSY を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_local_rwlock_try_lock_shared(com_util_local_rwlock_t *rwlock);
/**
 *  @brief          排他 (書き込み) ロックを取得します。
 *  @param[in,out]  rwlock     対象の読み書きロック。NULL を渡してはなりません。
 *  @param[in]      timeout_ms タイムアウト (ms)。@ref COM_UTIL_SYNC_WAIT_FOREVER または
 *                             @ref COM_UTIL_SYNC_NO_WAIT も指定可能です。
 *  @return         @ref COM_UTIL_SYNC_OK 、@ref COM_UTIL_SYNC_TIMEOUT 、
 *                  @ref COM_UTIL_SYNC_SYSTEM_ERROR のいずれかを返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_local_rwlock_lock_exclusive(com_util_local_rwlock_t *rwlock, uint32_t timeout_ms);
/**
 *  @brief          排他 (書き込み) ロックをノンブロッキングで取得試行します。
 *  @param[in,out]  rwlock  対象の読み書きロック。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_BUSY を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_local_rwlock_try_lock_exclusive(com_util_local_rwlock_t *rwlock);
/**
 *  @brief          共有 (読み取り) ロックを解放します。
 *  @param[in,out]  rwlock  対象の読み書きロック。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_SYSTEM_ERROR を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_local_rwlock_unlock_shared(com_util_local_rwlock_t *rwlock);
/**
 *  @brief          排他 (書き込み) ロックを解放します。
 *  @param[in,out]  rwlock  対象の読み書きロック。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_SYSTEM_ERROR を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_local_rwlock_unlock_exclusive(com_util_local_rwlock_t *rwlock);
/**
 *  @brief          読み書きロックを破棄します。
 *  @param[in,out]  rwlock  破棄する読み書きロック。NULL を渡してはなりません。
 */
COM_UTIL_EXPORT void COM_UTIL_API com_util_local_rwlock_destroy(com_util_local_rwlock_t *rwlock);

/**
 *  @brief          スレッドを生成して起動します。
 *  @param[out]     thread  生成したスレッドハンドルの格納先。NULL を渡してはなりません。
 *  @param[in]      func    スレッド関数。NULL を渡してはなりません。
 *  @param[in]      arg     @p func に渡す引数。NULL 可。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_SYSTEM_ERROR を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_thread_create(com_util_thread_t **thread, com_util_thread_func_t func, void *arg);
/**
 *  @brief          スレッドの終了を待機します。
 *  @param[in,out]  thread     対象のスレッドハンドル。NULL を渡してはなりません。
 *  @param[in]      timeout_ms タイムアウト (ms)。@ref COM_UTIL_SYNC_WAIT_FOREVER または
 *                             @ref COM_UTIL_SYNC_NO_WAIT も指定可能です。
 *  @return         @ref COM_UTIL_SYNC_OK 、@ref COM_UTIL_SYNC_TIMEOUT 、
 *                  @ref COM_UTIL_SYNC_SYSTEM_ERROR のいずれかを返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_thread_join(com_util_thread_t *thread, uint32_t timeout_ms);
/**
 *  @brief          スレッドを切り離します。切り離し後はリソースを自動解放します。
 *  @param[in,out]  thread  切り離すスレッドハンドル。NULL を渡してはなりません。
 */
COM_UTIL_EXPORT void COM_UTIL_API com_util_thread_detach(com_util_thread_t *thread);

/**
 *  @brief          識別子でプロセス横断ミューテックスを開きます (存在しない場合は生成)。
 *  @param[in]      identity  ロックを識別する名前文字列。NULL を渡してはなりません。
 *  @param[out]     lock      生成したロックハンドルの格納先。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_SYSTEM_ERROR を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_interprocess_lock_open(const char *identity, com_util_interprocess_lock_t **lock);
/**
 *  @brief          エクスポートされたディスクリプタからプロセス横断ミューテックスをインポートします。
 *  @param[in]      descriptor       インポートするディスクリプタデータ。NULL を渡してはなりません。
 *  @param[in]      descriptor_size  @p descriptor のサイズ (バイト)。
 *  @param[out]     lock             インポートしたロックハンドルの格納先。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK 、@ref COM_UTIL_SYNC_CORRUPT_DESCRIPTOR 、
 *                  @ref COM_UTIL_SYNC_SYSTEM_ERROR のいずれかを返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_interprocess_lock_import_descriptor(const void *descriptor, size_t descriptor_size,
                                             com_util_interprocess_lock_t **lock);
/**
 *  @brief          プロセス横断ミューテックスをディスクリプタにエクスポートします (プロセス間受け渡し用)。
 *  @param[in]      lock             エクスポートするロック。NULL を渡してはなりません。
 *  @param[out]     descriptor       エクスポート先バッファ。NULL の場合はサイズのみ取得します。
 *  @param[in,out]  descriptor_size  入力: @p descriptor のサイズ。出力: 必要サイズ。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK 、@ref COM_UTIL_SYNC_BUFFER_TOO_SMALL 、
 *                  @ref COM_UTIL_SYNC_SYSTEM_ERROR のいずれかを返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_interprocess_lock_export_descriptor(const com_util_interprocess_lock_t *lock,
                                             void *descriptor, size_t *descriptor_size);
/**
 *  @brief          プロセス横断ミューテックスをロックします。
 *  @param[in,out]  lock       対象のロック。NULL を渡してはなりません。
 *  @param[in]      timeout_ms タイムアウト (ms)。@ref COM_UTIL_SYNC_WAIT_FOREVER または
 *                             @ref COM_UTIL_SYNC_NO_WAIT も指定可能です。
 *  @return         @ref COM_UTIL_SYNC_OK 、@ref COM_UTIL_SYNC_TIMEOUT 、
 *                  @ref COM_UTIL_SYNC_SYSTEM_ERROR のいずれかを返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_interprocess_lock_lock(com_util_interprocess_lock_t *lock, uint32_t timeout_ms);
/**
 *  @brief          プロセス横断ミューテックスをノンブロッキングでロック試行します。
 *  @param[in,out]  lock  対象のロック。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_BUSY を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_interprocess_lock_try_lock(com_util_interprocess_lock_t *lock);
/**
 *  @brief          プロセス横断ミューテックスをアンロックします。
 *  @param[in,out]  lock  対象のロック。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_SYSTEM_ERROR を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_interprocess_lock_unlock(com_util_interprocess_lock_t *lock);
/**
 *  @brief          プロセス横断ミューテックスを破棄します。
 *  @param[in,out]  lock  破棄するロック。NULL を渡してはなりません。
 */
COM_UTIL_EXPORT void COM_UTIL_API com_util_interprocess_lock_destroy(com_util_interprocess_lock_t *lock);

/**
 *  @brief          識別子でプロセス横断読み書きロックを開きます (存在しない場合は生成)。
 *  @param[in]      identity  ロックを識別する名前文字列。NULL を渡してはなりません。
 *  @param[out]     lock      生成したロックハンドルの格納先。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_SYSTEM_ERROR を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_interprocess_rwlock_open(const char *identity, com_util_interprocess_rwlock_t **lock);
/**
 *  @brief          エクスポートされたディスクリプタからプロセス横断読み書きロックをインポートします。
 *  @param[in]      descriptor       インポートするディスクリプタデータ。NULL を渡してはなりません。
 *  @param[in]      descriptor_size  @p descriptor のサイズ (バイト)。
 *  @param[out]     lock             インポートしたロックハンドルの格納先。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK 、@ref COM_UTIL_SYNC_CORRUPT_DESCRIPTOR 、
 *                  @ref COM_UTIL_SYNC_SYSTEM_ERROR のいずれかを返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_interprocess_rwlock_import_descriptor(const void *descriptor, size_t descriptor_size,
                                               com_util_interprocess_rwlock_t **lock);
/**
 *  @brief          プロセス横断読み書きロックをディスクリプタにエクスポートします (プロセス間受け渡し用)。
 *  @param[in]      lock             エクスポートするロック。NULL を渡してはなりません。
 *  @param[out]     descriptor       エクスポート先バッファ。NULL の場合はサイズのみ取得します。
 *  @param[in,out]  descriptor_size  入力: @p descriptor のサイズ。出力: 必要サイズ。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK 、@ref COM_UTIL_SYNC_BUFFER_TOO_SMALL 、
 *                  @ref COM_UTIL_SYNC_SYSTEM_ERROR のいずれかを返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_interprocess_rwlock_export_descriptor(const com_util_interprocess_rwlock_t *lock,
                                               void *descriptor, size_t *descriptor_size);
/**
 *  @brief          プロセス横断共有 (読み取り) ロックを取得します。
 *  @param[in,out]  lock       対象のロック。NULL を渡してはなりません。
 *  @param[in]      timeout_ms タイムアウト (ms)。@ref COM_UTIL_SYNC_WAIT_FOREVER または
 *                             @ref COM_UTIL_SYNC_NO_WAIT も指定可能です。
 *  @return         @ref COM_UTIL_SYNC_OK 、@ref COM_UTIL_SYNC_TIMEOUT 、
 *                  @ref COM_UTIL_SYNC_SYSTEM_ERROR のいずれかを返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_interprocess_rwlock_lock_shared(com_util_interprocess_rwlock_t *lock, uint32_t timeout_ms);
/**
 *  @brief          プロセス横断共有 (読み取り) ロックをノンブロッキングで取得試行します。
 *  @param[in,out]  lock  対象のロック。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_BUSY を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_interprocess_rwlock_try_lock_shared(com_util_interprocess_rwlock_t *lock);
/**
 *  @brief          プロセス横断排他 (書き込み) ロックを取得します。
 *  @param[in,out]  lock       対象のロック。NULL を渡してはなりません。
 *  @param[in]      timeout_ms タイムアウト (ms)。@ref COM_UTIL_SYNC_WAIT_FOREVER または
 *                             @ref COM_UTIL_SYNC_NO_WAIT も指定可能です。
 *  @return         @ref COM_UTIL_SYNC_OK 、@ref COM_UTIL_SYNC_TIMEOUT 、
 *                  @ref COM_UTIL_SYNC_SYSTEM_ERROR のいずれかを返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_interprocess_rwlock_lock_exclusive(com_util_interprocess_rwlock_t *lock, uint32_t timeout_ms);
/**
 *  @brief          プロセス横断排他 (書き込み) ロックをノンブロッキングで取得試行します。
 *  @param[in,out]  lock  対象のロック。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_BUSY を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_interprocess_rwlock_try_lock_exclusive(com_util_interprocess_rwlock_t *lock);
/**
 *  @brief          プロセス横断読み書きロックを解放します。
 *  @param[in,out]  lock  対象のロック。NULL を渡してはなりません。
 *  @return         @ref COM_UTIL_SYNC_OK または @ref COM_UTIL_SYNC_SYSTEM_ERROR を返します。
 */
COM_UTIL_EXPORT com_util_sync_result_t COM_UTIL_API
com_util_interprocess_rwlock_unlock(com_util_interprocess_rwlock_t *lock);
/**
 *  @brief          プロセス横断読み書きロックを破棄します。
 *  @param[in,out]  lock  破棄するロック。NULL を渡してはなりません。
 */
COM_UTIL_EXPORT void COM_UTIL_API com_util_interprocess_rwlock_destroy(com_util_interprocess_rwlock_t *lock);

/**
 *  @brief          @p func をプロセス内で 1 回だけ呼び出します (スレッドセーフ)。
 *  @param[in,out]  flag  呼び出し状態を管理するフラグ。静的領域で 0 初期化して使用してください。
 *                        NULL を渡してはなりません。
 *  @param[in]      func  1 回だけ実行する関数。NULL を渡してはなりません。
 */
COM_UTIL_EXPORT void COM_UTIL_API com_util_call_once(com_util_once_flag_t *flag,
                                                     com_util_once_func_t func);

/**
 *  @brief      指定時間だけ現在のスレッドをスリープします。
 *  @param[in]  ms  スリープする時間 (ms)。0 を渡した場合は即時リターンします。
 */
COM_UTIL_EXPORT void COM_UTIL_API com_util_sleep_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COM_UTIL_SYNC_H */
