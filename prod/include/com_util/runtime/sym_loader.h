/**
 *******************************************************************************
 *  @file           sym_loader.h
 *  @brief          関数を動的に解決する symbol loader の公開 API を提供します。
 *  @author         c-modenization-kit sample team
 *  @date           2026/03/17
 *  @version        1.0.0
 *
 *  sym_loader は、テキスト設定ファイルから関数シンボルとライブラリ名を読み込み、
 *  実行時に動的リンクで関数を解決するキャッシュ機構です。
 *
 *  使用方法:
 *  1. com_util_sym_loader_entry を COM_UTIL_SYM_LOADER_ENTRY_INIT マクロで静的初期化します。
 *  2. com_util_sym_loader_init() でテキスト設定ファイルを読み込む (DllMain/constructor から呼ぶ)。
 *  3. com_util_sym_loader_resolve_as() で関数ポインターを取得して呼び出します。
 *  4. com_util_sym_loader_dispose() でリソースを解放する (DllMain/destructor から呼ぶ)。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_SYM_LOADER_H
#define COM_UTIL_SYM_LOADER_H

#include <com_util/base/platform.h>
#include <com_util/sync/sync.h>
#include <com_util/com_util_export.h>

#if defined(PLATFORM_LINUX)
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
    #endif /* _GNU_SOURCE */
#endif     /* PLATFORM_LINUX */

#include <stddef.h>

#if defined(PLATFORM_LINUX)
    #include <dlfcn.h>
#elif defined(PLATFORM_WINDOWS)
    #include <com_util/base/windows_sdk.h>
#endif /* PLATFORM_ */

/**
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#ifdef DOXYGEN
    /**
     *  @brief          Linux/Windows 共通のモジュール ハンドル型です。
     *
     *  sym_loader が内部で保持する動的ロード済みモジュールの不透明ハンドルです。\n
     *  Linux では `dlopen()` が返す `void *`、Windows では `LoadLibrary()` 系が返す
     *  `HMODULE` を表します。
     */
    #define COM_UTIL_MODULE_HANDLE void *
#elif defined(PLATFORM_LINUX)
    #define COM_UTIL_MODULE_HANDLE void *
#elif defined(PLATFORM_WINDOWS)
    #define COM_UTIL_MODULE_HANDLE HMODULE
#endif

#define COM_UTIL_SYM_LOADER_NAME_MAX 256 /**< lib_name / func_name 配列の最大長 (終端 '\0' を含む)。 */

    /**
     *  @brief          関数ポインター キャッシュ エントリです。
     *
     *  ライブラリ名・関数名・ハンドル・関数ポインターおよび排他制御用ロックを管理します。\n
     *  静的変数として定義する場合は COM_UTIL_SYM_LOADER_ENTRY_INIT マクロで初期化してください。
     */
    typedef struct com_util_sym_loader_entry
    {
        const char *func_key;                         /**< この関数インスタンスの識別キー。 */
        char lib_name[COM_UTIL_SYM_LOADER_NAME_MAX];  /**< 拡張子なしライブラリ名。[0]=='\0' = 未設定。 */
        char func_name[COM_UTIL_SYM_LOADER_NAME_MAX]; /**< 関数シンボル名。[0]=='\0' = 未設定。 */
        COM_UTIL_MODULE_HANDLE handle;                /**< キャッシュ済みハンドル (NULL = 未ロード)。 */
        void *func_ptr;                               /**< キャッシュ済み関数ポインター (NULL = 未取得)。 */
        int resolved;                                 /**< 解決済フラグ (0 = 未解決)。 */
        /* lock_state は __atomic_compare_exchange_n / InterlockedCompareExchange に渡すため、
           コーディング規範の例外として固定幅型 int32_t を維持する。 */
        volatile int32_t lock_state; /**< ロック初期化状態 (0=未初期化,1=初期化中,2=初期化済み)。 */
        com_util_local_lock *lock;   /**< ロード処理を保護するミューテックス。 */
    } com_util_sym_loader_entry;

/**
 *  @brief          com_util_sym_loader_entry 静的変数の初期化マクロです。
 *
 *  @param[in]      key     この関数インスタンスの識別キー (文字列リテラル)。
 *  @param[in]      type    格納する関数ポインターの型 (例: sample_func_t)。
 */
#define COM_UTIL_SYM_LOADER_ENTRY_INIT(key, type) {(key), {0}, {0}, NULL, NULL, 0, 0, NULL}

    /**
     *  @brief          拡張関数ポインターを返します (内部用)。
     *
     *  @param[in]      fobj com_util_sym_loader_entry へのポインター。
     *  @return         成功時 void * (関数ポインター)、失敗時 NULL。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  ロック フリーの fast path と per-entry mutex による double-checked locking で排他制御しており、複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT void *COM_UTIL_API com_util_sym_loader_resolve(com_util_sym_loader_entry *fobj);

/**
 *  @brief          拡張関数ポインターを返します。
 *
 *  @param[in]      fobj com_util_sym_loader_entry へのポインター。
 *  @param[in]      type COM_UTIL_SYM_LOADER_ENTRY_INIT で指定したものと同じ関数ポインター型。
 */
#define com_util_sym_loader_resolve_as(fobj, type) ((type)com_util_sym_loader_resolve(fobj))

    /**
     *  @brief          com_util_sym_loader_entry が明示的デフォルトかどうかを返します。
     *
     *  @param[in]      fobj com_util_sym_loader_entry へのポインター。
     *  @return         明示的デフォルトの場合は 1、それ以外は 0。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部で com_util_sym_loader_resolve を呼び出しており、排他制御はそちらに委譲します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_sym_loader_is_default(com_util_sym_loader_entry *fobj);

    /**
     *  @brief          com_util_sym_loader_entry ポインター配列を初期化します。
     *
     *  @param[in]      fobj_array  com_util_sym_loader_entry ポインター配列。
     *  @param[in]      fobj_length 配列の要素数。
     *  @param[in]      configpath  定義ファイルのパス。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  DLL ロード直後のシングル スレッド フェーズで呼び出してください。複数スレッドから同時に @p fobj_array の同一エントリへ書き込むと競合が発生します。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_sym_loader_init(com_util_sym_loader_entry *const *fobj_array,
                                                               const size_t fobj_length, const char *configpath);

    /**
     *  @brief          com_util_sym_loader_entry ポインター配列を解放します。
     *
     *  @param[in]      fobj_array  com_util_sym_loader_entry ポインター配列。
     *  @param[in]      fobj_length 配列の要素数。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  DllMain / destructor コンテキストのシングル スレッド フェーズで呼び出してください。他スレッドが resolve を実行中の場合、解放と競合します。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_sym_loader_dispose(com_util_sym_loader_entry *const *fobj_array,
                                                                  const size_t fobj_length);

    /**
     *  @brief          com_util_sym_loader_entry ポインター配列の内容を標準出力に表示します。
     *
     *  @param[in]      fobj_array      com_util_sym_loader_entry ポインター配列。
     *  @param[in]      fobj_length     配列の要素数。
     *  @return         すべてのエントリが正常に解決されている場合は 0、1 つでも失敗している場合は -1。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部で com_util_sym_loader_resolve を呼び出しており、排他制御はそちらに委譲します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_sym_loader_info(com_util_sym_loader_entry *const *fobj_array,
                                                              const size_t fobj_length);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_SYM_LOADER_H */
