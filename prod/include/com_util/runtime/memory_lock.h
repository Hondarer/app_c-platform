/**
 *******************************************************************************
 *  @file           memory_lock.h
 *  @brief          自プロセスのメモリ ロック API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/07/05
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_MEMORY_LOCK_H
#define COM_UTIL_MEMORY_LOCK_H

#include <stddef.h>
#include <com_util/base/platform.h>
#include <com_util/com_util_export.h>

/**
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define COM_UTIL_MEMORY_LOCK_CURRENT 0x01 /**< 現在マップ済みのメモリをロック対象にします。 */
#define COM_UTIL_MEMORY_LOCK_FUTURE  0x02 /**< 今後マップされるメモリもロック対象にします。 */
#define COM_UTIL_MEMORY_LOCK_ONFAULT 0x04 /**< ページ フォルト時にロックします。 */

    /**
     *  @brief          メモリ ロック操作の結果コードです。
     */
    typedef enum com_util_memory_lock_result_t
    {
        COM_UTIL_MEMORY_LOCK_OK = 0,                /**< 成功。 */
        COM_UTIL_MEMORY_LOCK_INVALID_ARGUMENT = 1,  /**< 引数が不正。 */
        COM_UTIL_MEMORY_LOCK_UNSUPPORTED = 2,       /**< 操作がサポートされない。 */
        COM_UTIL_MEMORY_LOCK_PERMISSION_DENIED = 3, /**< 権限不足。 */
        COM_UTIL_MEMORY_LOCK_LIMIT_EXCEEDED = 4,    /**< ロック可能量またはリソース上限を超過。 */
        COM_UTIL_MEMORY_LOCK_SYSTEM_ERROR = 5       /**< OS/システム エラー。 */
    } com_util_memory_lock_result_t;

    /**
     *  @brief          自プロセス メモリ ロックの解除情報です。
     *
     *  com_util_memory_lock_self() が成功した場合に生成され、呼び出し側が
     *  com_util_memory_lock_scope_release() へ渡して破棄します。\n
     *  本構造体のメンバーは公開されません。
     */
    typedef struct com_util_memory_lock_scope com_util_memory_lock_scope;

    /**
     *  @brief          自プロセス メモリ ロックのオプションです。
     */
    typedef struct com_util_memory_lock_self_options
    {
        int flags; /**< ロック対象を示す flag。0 または未知の bit を指定してはなりません。 */
#if defined(ARCH_X64)
        unsigned int pad; /**< x64 で stack_prefault_bytes のアラインメントを明示する予約領域。 */
#endif
        size_t stack_prefault_bytes; /**< ロック前に呼び出しスレッドで追加消費するスタック サイズ。0 可。 */
    } com_util_memory_lock_self_options;

    /**
     *  @brief          指定したメモリ範囲をロックします。
     *  @param[in]      address  ロック対象範囲の先頭アドレス。NULL を渡してはなりません。
     *  @param[in]      size     ロック対象範囲のサイズ (バイト)。0 を渡してはなりません。
     *  @return         結果コードを返します。
     *
     *  実際にロックされる範囲は OS のページ単位に丸められます。\n
     *  Windows では committed page だけをロックできます。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同じページに対する複数スレッドからの lock / unlock の対応関係は呼び出し側で管理してください。
     */
    COM_UTIL_EXPORT com_util_memory_lock_result_t COM_UTIL_API com_util_memory_lock_range(const void *address,
                                                                                          size_t size);

    /**
     *  @brief          指定したメモリ範囲のロックを解除します。
     *  @param[in]      address  解除対象範囲の先頭アドレス。NULL を渡してはなりません。
     *  @param[in]      size     解除対象範囲のサイズ (バイト)。0 を渡してはなりません。
     *  @return         結果コードを返します。
     *
     *  実際に解除される範囲は OS のページ単位に丸められます。\n
     *  Windows では部分的に未ロックのページを含むと失敗することがあります。
     */
    COM_UTIL_EXPORT com_util_memory_lock_result_t COM_UTIL_API com_util_memory_unlock_range(const void *address,
                                                                                            size_t size);

    /**
     *  @brief          自プロセスのメモリをまとめてロックします。
     *  @param[in]      options  ロック オプション。NULL を渡してはなりません。
     *  @param[out]     scope    解除情報の格納先。NULL を渡してはなりません。
     *  @return         結果コードを返します。
     *
     *  @p options の stack_prefault_bytes が 0 より大きい場合、ロック前に呼び出しスレッドのスタックを
     *  指定サイズ分だけ触ります。\n
     *  この処理は呼び出しスレッドだけを対象にします。\n
     *  Linux では mlockall() を使用します。\n
     *  Windows では @ref COM_UTIL_MEMORY_LOCK_CURRENT だけをサポートし、VirtualQuery() で現在の
     *  committed region を列挙して VirtualLock() を適用します。\n
     *  Windows で @ref COM_UTIL_MEMORY_LOCK_FUTURE または @ref COM_UTIL_MEMORY_LOCK_ONFAULT を指定した場合は
     *  @ref COM_UTIL_MEMORY_LOCK_UNSUPPORTED を返します。\n
     *  返された @p scope は com_util_memory_lock_scope_release() で破棄してください。
     *
     *  @par            呼び出しタイミング
     *  ロック対象のスタックを先に committed page にしたい場合は、@p options の stack_prefault_bytes を指定して
     *  本関数を呼び出します。\n
     *  関数が成功して @p scope が返った後、ページ フォルトを避けたい処理を実行します。
     *
    @startuml com_util_memory_lock_self のロック取得
        caption com_util_memory_lock_self のロック取得
        participant "呼び出し側" as Caller
        participant "com_util_memory_lock_self" as API
        participant "内部 lock" as Lock
        participant "OS" as OS

        Caller -> API : options, &scope
        API -> API : 引数と flag を検証
        opt stack_prefault_bytes > 0
            API -> API : 呼び出しスレッドのスタックを触る
        end
        API -> Lock : 取得
        alt Linux
            API -> OS : mlockall(flags)
            API -> API : self scope 数を加算
        else Windows
            API -> OS : VirtualQuery で committed region を列挙
            API -> OS : 未登録範囲へ VirtualLock
            API -> API : 登録済み範囲の ref-count を加算
        end
        API -> Lock : 解放
        API --> Caller : COM_UTIL_MEMORY_LOCK_OK, scope
    @enduml
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  複数スレッドから同時に呼び出して、呼び出しごとに独立した @p scope を取得できます。\n
     *  @p options の stack_prefault_bytes は呼び出しスレッドだけに作用します。
     */
    COM_UTIL_EXPORT com_util_memory_lock_result_t COM_UTIL_API
    com_util_memory_lock_self(const com_util_memory_lock_self_options *options, com_util_memory_lock_scope **scope);

    /**
     *  @brief          自プロセス全体ロックの解除情報を破棄し、ロックを解除します。
     *  @param[in]      scope  com_util_memory_lock_self() で取得した解除情報。NULL 可。
     *  @return         結果コードを返します。
     *
     *  NULL を渡した場合は何もせず @ref COM_UTIL_MEMORY_LOCK_OK を返します。
     *
     *  @par            解放タイミング
     *  @p scope は、対応する com_util_memory_lock_self() が成功した後、ロック状態を必要とする処理が終わった時点で
     *  1 回だけ解放します。\n
     *  Linux では最後の self scope が解放された時点で @c munlockall() を呼び出します。\n
     *  Windows では @p scope が保持する各範囲の ref-count を下げ、最後の参照がなくなった範囲だけ
     *  @c VirtualUnlock() を呼び出します。
     *
    @startuml com_util_memory_lock_scope_release のロック解放
        caption com_util_memory_lock_scope_release のロック解放
        participant "呼び出し側" as Caller
        participant "com_util_memory_lock_scope_release" as API
        participant "内部 lock" as Lock
        participant "OS" as OS

        Caller -> API : scope
        alt scope == NULL
            API --> Caller : COM_UTIL_MEMORY_LOCK_OK
        else scope != NULL
            API -> Lock : 取得
            alt Linux
                API -> API : self scope 数を減算
                opt self scope 数 == 0
                    API -> OS : munlockall()
                end
            else Windows
                loop scope の各範囲
                    API -> API : registry の ref-count を減算
                    opt ref-count == 0
                        API -> OS : VirtualUnlock()
                    end
                end
            end
            API -> Lock : 解放
            API -> API : scope を破棄
            API --> Caller : 結果コード
        end
    @enduml
     *
     *  @par            スレッド セーフ
     *  本関数は異なる @p scope に対してスレッド セーフです。\n
     *  同一 @p scope を複数スレッドから同時に渡してはなりません。\n
     *  同一 @p scope は 1 回だけ解放してください。
     */
    COM_UTIL_EXPORT com_util_memory_lock_result_t COM_UTIL_API
    com_util_memory_lock_scope_release(com_util_memory_lock_scope *scope);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_MEMORY_LOCK_H */
