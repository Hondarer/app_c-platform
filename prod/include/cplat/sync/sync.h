/**
 *******************************************************************************
 *  @file           sync.h
 *  @brief          スレッドと同期プリミティブを抽象化する API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/04/20
 *  @version        2.0.0
 *
 *  OS ごとの差異を隠蔽し、プロセス内同期とプロセス横断可能な
 *  アプリケーション排他を共通の結果コードで提供します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef CPLAT_SYNC_H
#define CPLAT_SYNC_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <cplat/base/result.h>
#include <cplat/cplat_export.h>

/**
 *  @ingroup        CPLAT_SYNC
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define CPLAT_SYNC_WAIT_FOREVER INT_MAX /**< タイムアウトなしで待機する (INT_MAX)。 */
#define CPLAT_SYNC_NO_WAIT      0       /**< 即時リターン (タイムアウト 0 ms)。 */

    /** @brief プロセス横断同期のバックエンド種別。 */
    typedef enum cplat_interprocess_sync_backend
    {
        CPLAT_INTERPROCESS_SYNC_BACKEND_LOCK_FILE = 1 /**< ロック ファイルを使用するバックエンド。 */
    } cplat_interprocess_sync_backend;

    typedef struct cplat_local_lock cplat_local_lock;                   /**< プロセス内ミューテックス。 */
    typedef struct cplat_condvar cplat_condvar;                         /**< プロセス内条件変数。 */
    typedef struct cplat_local_rwlock cplat_local_rwlock;               /**< プロセス内読み書きロック。 */
    typedef struct cplat_thread cplat_thread;                           /**< スレッド ハンドル。 */
    typedef struct cplat_interprocess_lock cplat_interprocess_lock;     /**< プロセス横断ミューテックス。 */
    typedef struct cplat_interprocess_rwlock cplat_interprocess_rwlock; /**< プロセス横断読み書きロック。 */

    /** スレッド関数ポインター型。 */
    typedef void (*cplat_thread_fn)(void *);
    /** call_once で 1 回だけ呼び出される関数ポインター型。 */
    typedef void (*cplat_once_fn)(void);

    /** call_once 状態。静的領域では 0 初期化して用いる。 */
    typedef struct cplat_once_flag
    {
        /* state は __atomic_compare_exchange_n / InterlockedCompareExchange に渡すため、
       コーディング規範の例外として固定幅型 int32_t を維持する。 */
        volatile int32_t state;
    } cplat_once_flag;

    /**
     *  @brief          プロセス内ミューテックスを生成します。
     *  @param[out]     mtx  生成したミューテックスの格納先。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_UNKNOWN を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  呼び出しごとに独立したミューテックスを生成し、内部に共有状態を持ちません。
     */
    CPLAT_EXPORT int CPLAT_API cplat_local_lock_create(cplat_local_lock **mtx);

    /**
     *  @brief          ミューテックスをロックします。
     *  @param[in]      mtx        対象のミューテックス。NULL を渡してはなりません。
     *  @param[in]      timeout_ms タイムアウト (ms)。@ref CPLAT_SYNC_WAIT_FOREVER または
     *                             @ref CPLAT_SYNC_NO_WAIT も指定可能です。
     *                             負値を渡した場合は @ref CPLAT_ERR_INVALID_ARGUMENT を返します。
     *  @return         @ref CPLAT_OK 、@ref CPLAT_ERR_TIMEOUT 、
     *                  @ref CPLAT_ERR_INVALID_ARGUMENT 、
     *                  @ref CPLAT_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p mtx に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_local_lock_lock(cplat_local_lock *mtx, int timeout_ms);

    /**
     *  @brief          ミューテックスを非ブロッキングでロック試行します。
     *  @param[in]      mtx  対象のミューテックス。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_BUSY を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p mtx に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_local_lock_try_lock(cplat_local_lock *mtx);

    /**
     *  @brief          ミューテックスをロック解除します。
     *  @param[in]      mtx  対象のミューテックス。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_UNKNOWN を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p mtx に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_local_lock_unlock(cplat_local_lock *mtx);

    /**
     *  @brief          ミューテックスを破棄します。
     *  @param[in]      mtx  破棄するミューテックス。NULL を渡してはなりません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  破棄対象の @p mtx を他スレッドが使用していないことを呼び出し側で保証してください。
     */
    CPLAT_EXPORT void CPLAT_API cplat_local_lock_dispose(cplat_local_lock *mtx);

    /**
     *  @brief          条件変数を生成します。
     *  @param[out]     cv  生成した条件変数の格納先。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_UNKNOWN を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  呼び出しごとに独立した条件変数を生成し、内部に共有状態を持ちません。
     */
    CPLAT_EXPORT int CPLAT_API cplat_condvar_create(cplat_condvar **cv);

    /**
     *  @brief          条件変数を待機します (@p mtx を atomically ロック解除後に待機し、シグナル受信後に再ロック)。
     *  @param[in]      cv         対象の条件変数。NULL を渡してはなりません。
     *  @param[in]      mtx        待機中にロック解除するミューテックス。NULL を渡してはなりません。
     *  @param[in]      timeout_ms タイムアウト (ms)。@ref CPLAT_SYNC_WAIT_FOREVER または
     *                             @ref CPLAT_SYNC_NO_WAIT も指定可能です。
     *                             負値を渡した場合は @ref CPLAT_ERR_INVALID_ARGUMENT を返します。
     *  @return         @ref CPLAT_OK 、@ref CPLAT_ERR_TIMEOUT 、
     *                  @ref CPLAT_ERR_INVALID_ARGUMENT 、
     *                  @ref CPLAT_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p cv を複数スレッドで待機できます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_condvar_wait(cplat_condvar *cv, cplat_local_lock *mtx,
                                                           int timeout_ms);

    /**
     *  @brief          待機中のスレッドを 1 つ起床させます。
     *  @param[in]      cv  対象の条件変数。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_UNKNOWN を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p cv に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_condvar_signal(cplat_condvar *cv);

    /**
     *  @brief          待機中のすべてのスレッドを起床させます。
     *  @param[in]      cv  対象の条件変数。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_UNKNOWN を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p cv に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_condvar_broadcast(cplat_condvar *cv);

    /**
     *  @brief          条件変数を破棄します。
     *  @param[in]      cv  破棄する条件変数。NULL を渡してはなりません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  破棄対象の @p cv を他スレッドが待機または通知に使用していないことを保証してください。
     */
    CPLAT_EXPORT void CPLAT_API cplat_condvar_dispose(cplat_condvar *cv);

    /**
     *  @brief          プロセス内読み書きロックを生成します。
     *  @param[out]     rwlock  生成した読み書きロックの格納先。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_UNKNOWN を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  呼び出しごとに独立した読み書きロックを生成し、内部に共有状態を持ちません。
     */
    CPLAT_EXPORT int CPLAT_API cplat_local_rwlock_create(cplat_local_rwlock **rwlock);

    /**
     *  @brief          共有 (読み取り) ロックを取得します。
     *  @param[in]      rwlock     対象の読み書きロック。NULL を渡してはなりません。
     *  @param[in]      timeout_ms タイムアウト (ms)。@ref CPLAT_SYNC_WAIT_FOREVER または
     *                             @ref CPLAT_SYNC_NO_WAIT も指定可能です。
     *                             負値を渡した場合は @ref CPLAT_ERR_INVALID_ARGUMENT を返します。
     *  @return         @ref CPLAT_OK 、@ref CPLAT_ERR_TIMEOUT 、
     *                  @ref CPLAT_ERR_INVALID_ARGUMENT 、
     *                  @ref CPLAT_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p rwlock に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_local_rwlock_lock_shared(cplat_local_rwlock *rwlock, int timeout_ms);

    /**
     *  @brief          共有 (読み取り) ロックを非ブロッキングで取得試行します。
     *  @param[in]      rwlock  対象の読み書きロック。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_BUSY を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p rwlock に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_local_rwlock_try_lock_shared(cplat_local_rwlock *rwlock);

    /**
     *  @brief          排他 (書き込み) ロックを取得します。
     *  @param[in]      rwlock     対象の読み書きロック。NULL を渡してはなりません。
     *  @param[in]      timeout_ms タイムアウト (ms)。@ref CPLAT_SYNC_WAIT_FOREVER または
     *                             @ref CPLAT_SYNC_NO_WAIT も指定可能です。
     *                             負値を渡した場合は @ref CPLAT_ERR_INVALID_ARGUMENT を返します。
     *  @return         @ref CPLAT_OK 、@ref CPLAT_ERR_TIMEOUT 、
     *                  @ref CPLAT_ERR_INVALID_ARGUMENT 、
     *                  @ref CPLAT_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p rwlock に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_local_rwlock_lock_exclusive(cplat_local_rwlock *rwlock,
                                                                          int timeout_ms);

    /**
     *  @brief          排他 (書き込み) ロックを非ブロッキングで取得試行します。
     *  @param[in]      rwlock  対象の読み書きロック。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_BUSY を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p rwlock に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_local_rwlock_try_lock_exclusive(cplat_local_rwlock *rwlock);

    /**
     *  @brief          共有 (読み取り) ロックを解放します。
     *  @param[in]      rwlock  対象の読み書きロック。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_UNKNOWN を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p rwlock に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_local_rwlock_unlock_shared(cplat_local_rwlock *rwlock);

    /**
     *  @brief          排他 (書き込み) ロックを解放します。
     *  @param[in]      rwlock  対象の読み書きロック。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_UNKNOWN を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p rwlock に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_local_rwlock_unlock_exclusive(cplat_local_rwlock *rwlock);

    /**
     *  @brief          読み書きロックを破棄します。
     *  @param[in]      rwlock  破棄する読み書きロック。NULL を渡してはなりません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  破棄対象の @p rwlock を他スレッドが使用していないことを呼び出し側で保証してください。
     */
    CPLAT_EXPORT void CPLAT_API cplat_local_rwlock_dispose(cplat_local_rwlock *rwlock);

    /**
     *  @brief          スレッドを生成して起動します。
     *  @param[out]     thread  生成したスレッド ハンドルの格納先。NULL を渡してはなりません。
     *  @param[in]      func    スレッド関数。NULL を渡してはなりません。
     *  @param[in]      arg     @p func に渡す引数。NULL 可。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_UNKNOWN を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  複数スレッドから同時に呼び出して独立したスレッドを生成できます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_thread_create(cplat_thread **thread, cplat_thread_fn func,
                                                            void *arg);

    /**
     *  @brief          スレッドの終了を待機します。
     *  @param[in]      thread     対象のスレッド ハンドル。NULL を渡してはなりません。
     *  @param[in]      timeout_ms タイムアウト (ms)。@ref CPLAT_SYNC_WAIT_FOREVER または
     *                             @ref CPLAT_SYNC_NO_WAIT も指定可能です。
     *                             負値を渡した場合は @ref CPLAT_ERR_INVALID_ARGUMENT を返します。
     *  @return         @ref CPLAT_OK 、@ref CPLAT_ERR_TIMEOUT 、
     *                  @ref CPLAT_ERR_INVALID_ARGUMENT 、
     *                  @ref CPLAT_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  同一 @p thread に対する並行呼び出しはスレッド セーフではありません。\n
     *  join 対象ごとに 1 スレッドだけが待機するように呼び出し側で制御してください。
     */
    CPLAT_EXPORT int CPLAT_API cplat_thread_join(cplat_thread *thread, int timeout_ms);

    /**
     *  @brief          スレッドを切り離します。切り離し後はリソースを自動解放します。
     *  @param[in]      thread  切り離すスレッド ハンドル。NULL を渡してはなりません。
     *
     *  @par            スレッド セーフ
     *  同一 @p thread に対する並行呼び出しはスレッド セーフではありません。\n
     *  detach は対象ハンドルごとに 1 回だけ実行してください。
     */
    CPLAT_EXPORT void CPLAT_API cplat_thread_detach(cplat_thread *thread);

    /**
     *  @brief          識別子でプロセス横断ミューテックスを開きます (存在しない場合は生成)。
     *  @param[in]      identity  ロックを識別する名前文字列。NULL を渡してはなりません。
     *  @param[out]     lock      生成したロック ハンドルの格納先。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_UNKNOWN を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p identity を複数スレッドから同時に指定しても OS の同期プリミティブを安全に取得できます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_interprocess_lock_open(const char *identity,
                                                                     cplat_interprocess_lock **lock);

    /**
     *  @brief          エクスポートされたディスクリプタからプロセス横断ミューテックスをインポートします。
     *  @param[in]      descriptor       インポートするディスクリプタ データ。NULL を渡してはなりません。
     *  @param[in]      descriptor_size  @p descriptor のサイズ (バイト)。
     *  @param[out]     lock             インポートしたロック ハンドルの格納先。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK 、@ref CPLAT_ERR_CORRUPT_DESCRIPTOR 、
     *                  @ref CPLAT_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  複数スレッドから独立したハンドルを同時にインポートできます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_interprocess_lock_import_descriptor(const void *descriptor,
                                                                                  size_t descriptor_size,
                                                                                  cplat_interprocess_lock **lock);

    /**
     *  @brief          プロセス横断ミューテックスをディスクリプタにエクスポートします (プロセス間受け渡し用)。
     *  @param[in]      lock             エクスポートするロック。NULL を渡してはなりません。
     *  @param[out]     descriptor       エクスポート先バッファー。NULL の場合はサイズのみ取得します。
     *  @param[in,out]  descriptor_size  入力: @p descriptor のサイズ。出力: 必要サイズ。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK 、@ref CPLAT_ERR_BUFFER_TOO_SMALL 、
     *                  @ref CPLAT_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  共有状態を変更せずにディスクリプタを出力するため、同一 @p lock に対して並行呼び出しできます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_interprocess_lock_export_descriptor(
        const cplat_interprocess_lock *lock, void *descriptor, size_t *descriptor_size);

    /**
     *  @brief          プロセス横断ミューテックスをロックします。
     *  @param[in]      lock       対象のロック。NULL を渡してはなりません。
     *  @param[in]      timeout_ms タイムアウト (ms)。@ref CPLAT_SYNC_WAIT_FOREVER または
     *                             @ref CPLAT_SYNC_NO_WAIT も指定可能です。
     *                             負値を渡した場合は @ref CPLAT_ERR_INVALID_ARGUMENT を返します。
     *  @return         @ref CPLAT_OK 、@ref CPLAT_ERR_TIMEOUT 、
     *                  @ref CPLAT_ERR_INVALID_ARGUMENT 、
     *                  @ref CPLAT_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p lock に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_interprocess_lock_lock(cplat_interprocess_lock *lock, int timeout_ms);

    /**
     *  @brief          プロセス横断ミューテックスを非ブロッキングでロック試行します。
     *  @param[in]      lock  対象のロック。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_BUSY を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p lock に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_interprocess_lock_try_lock(cplat_interprocess_lock *lock);

    /**
     *  @brief          プロセス横断ミューテックスをロック解除します。
     *  @param[in]      lock  対象のロック。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_UNKNOWN を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p lock に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_interprocess_lock_unlock(cplat_interprocess_lock *lock);

    /**
     *  @brief          プロセス横断ミューテックスを破棄します。
     *  @param[in]      lock  破棄するロック。NULL を渡してはなりません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  破棄対象の @p lock を他スレッドや他プロセスが使用していないことを確認してから呼び出してください。
     */
    CPLAT_EXPORT void CPLAT_API cplat_interprocess_lock_dispose(cplat_interprocess_lock *lock);

    /**
     *  @brief          識別子でプロセス横断読み書きロックを開きます (存在しない場合は生成)。
     *  @param[in]      identity  ロックを識別する名前文字列。NULL を渡してはなりません。
     *  @param[out]     lock      生成したロック ハンドルの格納先。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_UNKNOWN を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p identity を複数スレッドから同時に指定しても OS の同期プリミティブを安全に取得できます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_interprocess_rwlock_open(const char *identity,
                                                                       cplat_interprocess_rwlock **lock);

    /**
     *  @brief          エクスポートされたディスクリプタからプロセス横断読み書きロックをインポートします。
     *  @param[in]      descriptor       インポートするディスクリプタ データ。NULL を渡してはなりません。
     *  @param[in]      descriptor_size  @p descriptor のサイズ (バイト)。
     *  @param[out]     lock             インポートしたロック ハンドルの格納先。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK 、@ref CPLAT_ERR_CORRUPT_DESCRIPTOR 、
     *                  @ref CPLAT_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  複数スレッドから独立したハンドルを同時にインポートできます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_interprocess_rwlock_import_descriptor(
        const void *descriptor, size_t descriptor_size, cplat_interprocess_rwlock **lock);

    /**
     *  @brief          プロセス横断読み書きロックをディスクリプタにエクスポートします (プロセス間受け渡し用)。
     *  @param[in]      lock             エクスポートするロック。NULL を渡してはなりません。
     *  @param[out]     descriptor       エクスポート先バッファー。NULL の場合はサイズのみ取得します。
     *  @param[in,out]  descriptor_size  入力: @p descriptor のサイズ。出力: 必要サイズ。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK 、@ref CPLAT_ERR_BUFFER_TOO_SMALL 、
     *                  @ref CPLAT_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  共有状態を変更せずにディスクリプタを出力するため、同一 @p lock に対して並行呼び出しできます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_interprocess_rwlock_export_descriptor(
        const cplat_interprocess_rwlock *lock, void *descriptor, size_t *descriptor_size);

    /**
     *  @brief          プロセス横断共有 (読み取り) ロックを取得します。
     *  @param[in]      lock       対象のロック。NULL を渡してはなりません。
     *  @param[in]      timeout_ms タイムアウト (ms)。@ref CPLAT_SYNC_WAIT_FOREVER または
     *                             @ref CPLAT_SYNC_NO_WAIT も指定可能です。
     *                             負値を渡した場合は @ref CPLAT_ERR_INVALID_ARGUMENT を返します。
     *  @return         @ref CPLAT_OK 、@ref CPLAT_ERR_TIMEOUT 、
     *                  @ref CPLAT_ERR_INVALID_ARGUMENT 、
     *                  @ref CPLAT_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p lock に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_interprocess_rwlock_lock_shared(cplat_interprocess_rwlock *lock,
                                                                              int timeout_ms);

    /**
     *  @brief          プロセス横断共有 (読み取り) ロックを非ブロッキングで取得試行します。
     *  @param[in]      lock  対象のロック。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_BUSY を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p lock に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_interprocess_rwlock_try_lock_shared(cplat_interprocess_rwlock *lock);

    /**
     *  @brief          プロセス横断排他 (書き込み) ロックを取得します。
     *  @param[in]      lock       対象のロック。NULL を渡してはなりません。
     *  @param[in]      timeout_ms タイムアウト (ms)。@ref CPLAT_SYNC_WAIT_FOREVER または
     *                             @ref CPLAT_SYNC_NO_WAIT も指定可能です。
     *                             負値を渡した場合は @ref CPLAT_ERR_INVALID_ARGUMENT を返します。
     *  @return         @ref CPLAT_OK 、@ref CPLAT_ERR_TIMEOUT 、
     *                  @ref CPLAT_ERR_INVALID_ARGUMENT 、
     *                  @ref CPLAT_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p lock に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_interprocess_rwlock_lock_exclusive(cplat_interprocess_rwlock *lock,
                                                                                 int timeout_ms);

    /**
     *  @brief          プロセス横断排他 (書き込み) ロックを非ブロッキングで取得試行します。
     *  @param[in]      lock  対象のロック。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_BUSY を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p lock に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API
    cplat_interprocess_rwlock_try_lock_exclusive(cplat_interprocess_rwlock *lock);

    /**
     *  @brief          プロセス横断読み書きロックを解放します。
     *  @param[in]      lock  対象のロック。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK または @ref CPLAT_ERR_UNKNOWN を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p lock に対して複数スレッドから同時に呼び出せます。
     */
    CPLAT_EXPORT int CPLAT_API cplat_interprocess_rwlock_unlock(cplat_interprocess_rwlock *lock);

    /**
     *  @brief          プロセス横断読み書きロックを破棄します。
     *  @param[in]      lock  破棄するロック。NULL を渡してはなりません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  破棄対象の @p lock を他スレッドや他プロセスが使用していないことを確認してから呼び出してください。
     */
    CPLAT_EXPORT void CPLAT_API cplat_interprocess_rwlock_dispose(cplat_interprocess_rwlock *lock);

    /**
     *  @brief          @p func をプロセス内で 1 回だけ呼び出します。
     *  @param[in]      flag  呼び出し状態を管理するフラグ。静的領域で 0 初期化して使用してください。
     *                        NULL を渡してはなりません。
     *  @param[in]      func  1 回だけ実行する関数。NULL を渡してはなりません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p flag を複数スレッドから同時に指定しても @p func は 1 回だけ実行されます。
     */
    CPLAT_EXPORT void CPLAT_API cplat_call_once(cplat_once_flag *flag, cplat_once_fn func);

    /**
     *  @brief      指定時間だけ現在のスレッドをスリープします。
     *  @param[in]  ms  スリープする時間 (ms)。0 以下の値を渡した場合は即時リターンします (no-op)。
     *
     *  @note           Linux では、待機中にシグナルを受信した場合も、
     *                  残り時間を再計算して待機を継続するため、規定時間が経過するまで待機します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  呼び出し元スレッドだけを待機させ、ライブラリ内部の共有状態を変更しません。
     */
    CPLAT_EXPORT void CPLAT_API cplat_sleep_ms(int ms);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* CPLAT_SYNC_H */
