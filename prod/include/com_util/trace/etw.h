#ifndef COM_UTIL_ETW_H
#define COM_UTIL_ETW_H

#include <com_util/base/platform.h>
#include <com_util/base/result.h>
#include <com_util/com_util_export.h>
#include <stdint.h>

/**
 *  @ingroup        COM_UTIL_TRACE
 *  @{
 */

/**
 *  @file           etw.h
 *  @brief          ETW を使用したトレースを補助する API を提供します。
 *
 *  TraceLogging ベースの ETW プロバイダーを簡易に操作するための
 *  ヘルパー関数群を提供します。\n
 *  Windows 専用ライブラリです。呼び出し元は `#if defined(PLATFORM_WINDOWS)` の
 *  中でのみ使用してください。
 *
 *  @hideincludedbygraph
 *
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

/**
 *  @struct         com_util_etw_event
 *  @brief          ETW consumer が受け取るイベント情報です。
 *
 *  event_name / service / message は callback 呼び出し中のみ有効なポインターです。
 */
typedef struct com_util_etw_event
{
    /** イベント レベル (1-5)。 */
    int level;
    /** イベント発行元のプロセス ID (Windows DWORD 互換のためコーディング規範の例外として uint32_t を維持)。 */
    uint32_t process_id;
    /** イベント名。取得できない場合は NULL。 */
    const char *event_name;
    /** Service フィールド。存在しない場合は NULL。 */
    const char *service;
    /** Message フィールド。存在しない場合は NULL。 */
    const char *message;
    /** ETW が付与したタイムスタンプ。EVENT_HEADER::TimeStamp の生値 (100ns 単位)。 */
    int64_t timestamp_100ns;
} com_util_etw_event;

/**
 *  @typedef        com_util_etw_event_fn
 *  @brief          ETW イベント受信コールバック型です。
 *
 *  @param[in]      event    受信イベント。callback 呼び出し中のみ参照可能。
 *  @param[in]      context  com_util_etw_session_start に渡したユーザー データ。
 *
 *  @par            スレッド セーフ
 *  コールバックは ETW ワーカー スレッドから呼び出されます。\n
 *  コールバックの実装者は再入性を確保してください。
 */
typedef void (*com_util_etw_event_fn)(const com_util_etw_event *event, void *context);

#if defined(PLATFORM_WINDOWS)

    #include <com_util/base/windows_sdk.h>

/* ===== プロバイダー参照型 ===== */

/**
 *  @typedef        com_util_etw_provider_ref_t
 *  @brief          プロバイダー参照型です。
 *
 *  TraceLoggingHProvider (TraceLoggingProvider.h が定義する型) と同等です。
 */
struct _tlgProvider_t;
typedef struct _tlgProvider_t const *com_util_etw_provider_ref_t;

    /* ===== プロバイダー定義マクロ ===== */

    /**
     *  @brief          ETW プロバイダーを定義するマクロです。
     *
     *  呼び出し元の .c ファイルのファイル スコープに 1 回だけ記述します。\n
     *  TRACELOGGING_DEFINE_PROVIDER(var, name, guid) に展開します。
     *
     *  @param          var   プロバイダー変数名 (com_util_etw_provider_ref_t 型)
     *  @param          name  プロバイダー名 (文字列リテラル)
     *  @param          guid  GUID (TraceLogging 形式の括弧付き定数タプル)
     */
    #define COM_UTIL_ETW_DEFINE_PROVIDER(var, name, guid) TRACELOGGING_DEFINE_PROVIDER(var, name, guid)

/* ===== 不透明ハンドル型 ===== */

/** ETW プロバイダー ハンドル (不透明型)。 */
typedef struct com_util_etw_provider com_util_etw_provider;

    /* ===== API 関数 ===== */

    #ifdef __cplusplus
extern "C"
{
    #endif /* __cplusplus */

    /**
     *  @brief          ETW プロバイダーを登録します。
     *
     *  @param[in]      provider_ref  COM_UTIL_ETW_DEFINE_PROVIDER で定義した変数。
     *  @return         成功時はハンドル、失敗時は NULL を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT com_util_etw_provider *COM_UTIL_API
    com_util_etw_provider_create(com_util_etw_provider_ref_t provider_ref);

    /**
     *  @brief          ETW プロバイダーへ UTF-8 メッセージを書き込みます。
     *
     *  @param[in]      handle   com_util_etw_provider_create の戻り値。NULL は無視。
     *  @param[in]      level    イベント レベル (1=CRITICAL / 2=ERROR / 3=WARNING / 4=INFO / 5=VERBOSE)。
     *  @param[in]      service  サービス名です。NULL の場合は Service フィールドなしで書き込みます。
     *  @param[in]      message  null 終端 UTF-8 文字列。NULL は無視。
     *  @return         常に @ref COM_UTIL_OK を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  TraceLoggingWrite は複数スレッドからの同時呼び出しをサポートしています。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_etw_provider_write(com_util_etw_provider *handle, int level,
                                                                 const char *service, const char *message);

    /**
     *  @brief          ETW プロバイダーの登録を解除します。
     *
     *  @param[in]      handle   com_util_etw_provider_create の戻り値。NULL は無視。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  解放対象の @p handle を他スレッドが使用していないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_etw_provider_dispose(com_util_etw_provider *handle);

    /* ===== セッション (Consumer) API ===== */

    /** ETW セッション ハンドル (不透明型)。 */
    typedef struct com_util_etw_session com_util_etw_session;

    /**
     *  @brief          ETW セッション開始に必要な権限があるか検査します。
     *
     *  @return         必要な権限がある場合は @ref COM_UTIL_OK 、権限不足の場合は
     *                  @ref COM_UTIL_ERR_PERMISSION_DENIED 、検査中にシステム エラーが発生した場合は
     *                  @ref COM_UTIL_ERR_UNKNOWN を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_etw_session_check_access(void);

    /**
     *  @brief          リアルタイム ETW セッションを開始し、指定プロバイダーを購読します。
     *
     *  @param[in]      session_name       セッション名 (システム全体で一意にすること)。
     *  @param[in]      provider_guid_str  GUID 文字列 "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"。
     *  @param[in]      callback           イベント受信時に呼ばれるコールバック。
     *  @param[in]      context            コールバックに渡すユーザー データ。
     *  @param[out]     session_out        開始したセッションのハンドルの格納先。NULL を渡してはなりません。\n
     *                  失敗時は NULL を格納します。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_PERMISSION_DENIED 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。各呼び出しは独立したセッションを生成します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_etw_session_start(const char *session_name, const char *provider_guid_str,
                                                                com_util_etw_event_fn callback, void *context,
                                                                com_util_etw_session **session_out);

    /**
     *  @brief          ETW セッションを停止し、リソースを解放します。
     *
     *  @param[in]      session  com_util_etw_session_start の戻り値。NULL は無視。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  同一 @p session への並行呼び出しは未定義動作です。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_etw_session_stop(com_util_etw_session *session);

    #ifdef __cplusplus
}
    #endif /* __cplusplus */

#endif /* PLATFORM_WINDOWS */

/** @} */

#endif /* COM_UTIL_ETW_H */
