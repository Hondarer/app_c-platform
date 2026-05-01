#ifndef COM_UTIL_ETW_H
#define COM_UTIL_ETW_H

#include <com_util/base/platform.h>
#include <com_util_export.h>
#include <stdint.h>

/**
 *  @file           etw.h
 *  @brief          ETW (Event Tracing for Windows) ヘルパーライブラリ。
 *  @details        TraceLogging ベースの ETW プロバイダを簡易に操作するための
 *                  ヘルパー関数群を提供します。\n
 *                  Windows 専用ライブラリです。呼び出し元は @c \#if defined(PLATFORM_WINDOWS) の
 *                  中でのみ使用してください。
 */

/**
 *  @struct         com_util_etw_event_t
 *  @brief          ETW consumer が受け取るイベント情報。
 *  @details        event_name / service / message は callback 呼び出し中のみ有効なポインタです。
 */
typedef struct com_util_etw_event_t
{
    /** イベントレベル (1-5)。 */
    int level;
    /** イベント発行元のプロセス ID。 */
    uint32_t process_id;
    /** イベント名。取得できない場合は NULL。 */
    const char *event_name;
    /** Service フィールド。存在しない場合は NULL。 */
    const char *service;
    /** Message フィールド。存在しない場合は NULL。 */
    const char *message;
    /** ETW が付与したタイムスタンプ。EVENT_HEADER::TimeStamp の生値 (100ns 単位)。 */
    int64_t timestamp_100ns;
} com_util_etw_event_t;

/**
 *  @typedef        com_util_etw_event_callback_t
 *  @brief          ETW イベント受信コールバック型。
 *
 *  @param[in]      event    受信イベント。callback 呼び出し中のみ参照可能。
 *  @param[in]      context  com_util_etw_session_start に渡したユーザーデータ。
 */
typedef void (*com_util_etw_event_callback_t)(const com_util_etw_event_t *event, void *context);

#if defined(PLATFORM_WINDOWS)

#include <com_util/base/windows_sdk.h>

/* ===== プロバイダ参照型 ===== */

/**
 *  @typedef        com_util_etw_provider_ref_t
 *  @brief          プロバイダ参照型。
 *  @details        TraceLoggingHProvider (TraceLoggingProvider.h が定義する型) と同等です。
 */
struct _tlgProvider_t;
typedef struct _tlgProvider_t const *com_util_etw_provider_ref_t;

/* ===== プロバイダ定義マクロ ===== */

/**
 *  @brief          ETW プロバイダを定義するマクロ。
 *  @details        呼び出し元の .c ファイルのファイルスコープに 1 回だけ記述します。\n
 *                  TRACELOGGING_DEFINE_PROVIDER(var, name, guid) に展開します。
 *
 *  @param          var   プロバイダ変数名 (com_util_etw_provider_ref_t 型)
 *  @param          name  プロバイダ名 (文字列リテラル)
 *  @param          guid  GUID (TraceLogging 形式の括弧付き定数タプル)
 */
#define COM_UTIL_ETW_DEFINE_PROVIDER(var, name, guid) \
    TRACELOGGING_DEFINE_PROVIDER(var, name, guid)

/* ===== 不透明ハンドル型 ===== */

/** ETW プロバイダハンドル (不透明型)。 */
typedef struct com_util_etw_provider com_util_etw_provider_t;

/* ===== API 関数 ===== */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          ETW プロバイダを登録する。
     *
     *  @param[in]      provider_ref  COM_UTIL_ETW_DEFINE_PROVIDER で定義した変数。
     *  @return         成功時: ハンドル。失敗時: NULL。
     */
    COM_UTIL_EXPORT com_util_etw_provider_t *COM_UTIL_API
        com_util_etw_provider_create(com_util_etw_provider_ref_t provider_ref);

    /**
     *  @brief          ETW プロバイダへ UTF-8 メッセージを書き込む。
     *
     *  @param[in]      handle   com_util_etw_provider_create の戻り値。NULL は無視。
     *  @param[in]      level    イベントレベル (1=CRITICAL / 2=ERROR / 3=WARNING / 4=INFO / 5=VERBOSE)。
     *  @param[in]      service  サービス名。NULL の場合は Service フィールドなしで書き込む。
     *  @param[in]      message  null 終端 UTF-8 文字列。NULL は無視。
     *  @return         成功 0 / 失敗 -1。
     */
    COM_UTIL_EXPORT int COM_UTIL_API
        com_util_etw_provider_write(com_util_etw_provider_t *handle, int level,
                           const char *service, const char *message);

    /**
     *  @brief          ETW プロバイダの登録を解除する。
     *
     *  @param[in]      handle   com_util_etw_provider_create の戻り値。NULL は無視。
     */
    COM_UTIL_EXPORT void COM_UTIL_API
        com_util_etw_provider_dispose(com_util_etw_provider_t *handle);

    /* ===== セッション (Consumer) API ===== */

    /** @name ETW セッションステータスコード */
    /** @{ */

    /** 成功。 */
    #define COM_UTIL_ETW_SESSION_OK          0
    /** パラメータエラー (NULL または不正な GUID)。 */
    #define COM_UTIL_ETW_SESSION_ERR_PARAM  -1
    /** 権限不足 (Administrators または Performance Log Users が必要)。 */
    #define COM_UTIL_ETW_SESSION_ERR_ACCESS -2
    /** その他のシステムエラー。 */
    #define COM_UTIL_ETW_SESSION_ERR_SYSTEM -3

    /** @} */

    /** ETW セッションハンドル (不透明型)。 */
    typedef struct com_util_etw_session com_util_etw_session_t;

    /**
     *  @brief          ETW セッション開始に必要な権限があるか検査する。
     *
     *  @return         COM_UTIL_ETW_SESSION_OK / COM_UTIL_ETW_SESSION_ERR_ACCESS /
     *                  COM_UTIL_ETW_SESSION_ERR_SYSTEM。
     */
    COM_UTIL_EXPORT int COM_UTIL_API
        com_util_etw_session_check_access(void);

    /**
     *  @brief          リアルタイム ETW セッションを開始し、指定プロバイダを購読する。
     *
     *  @param[in]      session_name       セッション名 (システム全体で一意にすること)。
     *  @param[in]      provider_guid_str  GUID 文字列 "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"。
     *  @param[in]      callback           イベント受信時に呼ばれるコールバック。
     *  @param[in]      context            コールバックに渡すユーザーデータ。
     *  @param[out]     out_status         エラーコード出力先 (NULL 可)。
     *  @return         成功: セッションハンドル / 失敗: NULL。
     */
    COM_UTIL_EXPORT com_util_etw_session_t *COM_UTIL_API
        com_util_etw_session_start(const char *session_name,
                          const char *provider_guid_str,
                          com_util_etw_event_callback_t callback,
                          void *context,
                          int *out_status);

    /**
     *  @brief          ETW セッションを停止し、リソースを解放する。
     *
     *  @param[in]      session  com_util_etw_session_start の戻り値。NULL は無視。
     */
    COM_UTIL_EXPORT void COM_UTIL_API
        com_util_etw_session_stop(com_util_etw_session_t *session);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PLATFORM_WINDOWS */

#endif /* COM_UTIL_ETW_H */
