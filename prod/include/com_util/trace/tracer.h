/**
 *******************************************************************************
 *  @file           tracer.h
 *  @brief          クロスプラットフォーム トレーシング API。
 *  @author         Tetsuo Honda
 *  @date           2026/03/05
 *  @version        1.0.0
 *
 *  Windows (ETW) と Linux (syslog) の差異を抽象化し、
 *  共通のトレースレベルとインターフェースを提供します。\n
 *  内部で @c com_util/trace/etw.h (Windows) または @c com_util/trace/syslog.h (Linux) を
 *  使用します。
 *
 *  @par            アーキテクチャ
 *  @code
   Application
         |
         v           tracer.h (共通 API)
         |
   +-----+-----+------+--------+
   |           |      |        |
   ETW        syslog File    stderr
   (Windows)  (Linux) (両OS)  (両OS)
 *  @endcode
 *
 *  @par            使用例 (共通)
 *  @code{.c}
   #include <com_util/trace/tracer.h>

   com_util_tracer_t *tracer = com_util_tracer_create();
   com_util_tracer_set_name(tracer, "myapp", 0);
   com_util_tracer_start(tracer);
   com_util_tracer_write(tracer, COM_UTIL_TRACE_LEVEL_INFO, NULL, "application started");
   com_util_tracer_stop(tracer);
   com_util_tracer_dispose(tracer);
 *  @endcode
 *
 *  @par            使用例 (設定変更)
 *  @code{.c}
   com_util_tracer_t *tracer = com_util_tracer_create();
   com_util_tracer_set_name(tracer, "myapp", 0);
   com_util_tracer_set_os_level(tracer, COM_UTIL_TRACE_LEVEL_VERBOSE);
   com_util_tracer_start(tracer);
   com_util_tracer_write(tracer, COM_UTIL_TRACE_LEVEL_INFO, NULL, "running as myapp");
   com_util_tracer_stop(tracer);
   com_util_tracer_set_name(tracer, "myapp", 1); // "myapp-1" として再開
   com_util_tracer_start(tracer);
   com_util_tracer_write(tracer, COM_UTIL_TRACE_LEVEL_INFO, NULL, "running as myapp-1");
   com_util_tracer_stop(tracer);
   com_util_tracer_dispose(tracer);
 *  @endcode
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef COM_UTIL_TRACER_H
#define COM_UTIL_TRACER_H

/* size_t (com_util_tracer_write_hex / com_util_tracer_write_hexf で使用) */
#include <stddef.h>
/* int64_t (com_util_tracer_set_name で使用) */
#include <inttypes.h>
/* strrchr (_com_util_tracer_basename マクロで使用) */
#include <string.h>
#include <com_util/base/platform.h>
#include <com_util/clock/clock.h>
#include <com_util/crt/path.h>
#include <com_util_export.h>

/* 内部で使用するプラットフォーム固有ヘッダー */
#if defined(PLATFORM_LINUX)
#include <com_util/trace/syslog.h>
#elif defined(PLATFORM_WINDOWS)
#include <com_util/trace/etw.h>
#endif /* PLATFORM_ */

/* ===== デフォルトプロバイダ定義 (Windows) ===== */

#if defined(PLATFORM_WINDOWS)

/**
 *  @def            COM_UTIL_TRACER_DEFAULT_PROVIDER_NAME
 *  @brief          com_util_tracer_create が使用するデフォルト ETW プロバイダ名。
 */
#define COM_UTIL_TRACER_DEFAULT_PROVIDER_NAME "Company.Product"

/**
 *  @def            COM_UTIL_TRACER_DEFAULT_PROVIDER_GUID
 *  @brief          デフォルト ETW プロバイダの GUID (TraceLogging タプル形式)。
 *
 *  TRACELOGGING_DEFINE_PROVIDER で使用する形式です。
 */
#define COM_UTIL_TRACER_DEFAULT_PROVIDER_GUID \
    (0xc3a7b5d1, 0x4e2f, 0x4a89, 0x96, 0xc8, 0xd7, 0xe9, 0xf1, 0xa2, 0xb3, 0xc4)

/**
 *  @def            COM_UTIL_TRACER_DEFAULT_PROVIDER_GUID_STR
 *  @brief          デフォルト ETW プロバイダの GUID (文字列形式)。
 *
 *  com_util_etw_session_start に渡す場合など、文字列形式の GUID が
 *  必要な場面で使用します。
 */
#define COM_UTIL_TRACER_DEFAULT_PROVIDER_GUID_STR \
    "c3a7b5d1-4e2f-4a89-96c8-d7e9f1a2b3c4"

#endif /* PLATFORM_WINDOWS */

/* ===== メッセージ長制限 ===== */

/**
 *  @def            COM_UTIL_TRACER_MESSAGE_MAX_BYTES
 *  @brief          com_util_tracer_write が受け付けるメッセージの最大バイト数 (null 終端含む)。
 *
 *  ETW (約 65,000 バイト) と syslog (RFC 3164: 1,024 バイト) の
 *  推奨上限のうち小さい方を採用し、クロスプラットフォームでの
 *  安全な転送を保証します。\n
 *  本文の最大長は @c COM_UTIL_TRACER_MESSAGE_MAX_BYTES @c - @c 1 (= 1,023) バイトです。
 */
#define COM_UTIL_TRACER_MESSAGE_MAX_BYTES 1024

/**
 *  @def            COM_UTIL_TRACER_HEX_MAX_DATA_BYTES
 *  @brief          com_util_tracer_write_hex がラベルなしで HEX 出力できるバイナリデータの最大バイト数。
 *
 *  1 バイトあたり 3 文字 (HH + スペース) を消費し、最終バイトは 2 文字です。\n
 *  ラベル (@p message) を指定した場合はラベル長 + セパレータ (": ") 分だけ
 *  出力可能なバイナリデータ量が減少します。\n
 *  データがこの上限を超える場合は切り詰めが行われ、
 *  末尾に @c "..." が付与されます。
 */
#define COM_UTIL_TRACER_HEX_MAX_DATA_BYTES 341

/* ===== 共通トレースレベル ===== */

/**
 *  @enum           com_util_trace_level_t
 *  @brief          アプリケーション共通トレースレベル。
 *
 *  OS 非依存のトレースレベルを定義します。重大度は上から下へ低下します。\n
 *  内部で ETW Level (1-5) および syslog severity へマッピングされます。\n
 *  COM_UTIL_TRACE_LEVEL_DEBUG は ETW / syslog では COM_UTIL_TRACE_LEVEL_VERBOSE と同じ詳細度で扱われます。
 *
 *  | com_util_trace_level          | ETW Level         | syslog severity |
 *  | --------------------------- | ----------------- | --------------- |
 *  | COM_UTIL_TRACE_LEVEL_CRITICAL | Critical (1)      | LOG_CRIT (2)    |
 *  | COM_UTIL_TRACE_LEVEL_ERROR    | Error (2)         | LOG_ERR (3)     |
 *  | COM_UTIL_TRACE_LEVEL_WARNING  | Warning (3)       | LOG_WARNING (4) |
 *  | COM_UTIL_TRACE_LEVEL_INFO     | Informational (4) | LOG_INFO (6)    |
 *  | COM_UTIL_TRACE_LEVEL_VERBOSE  | Verbose (5)       | LOG_DEBUG (7)   |
 *  | COM_UTIL_TRACE_LEVEL_DEBUG    | Verbose (5)       | LOG_DEBUG (7)   |
 */
typedef enum com_util_trace_level_t
{
    COM_UTIL_TRACE_LEVEL_CRITICAL = 0, /**< 致命的エラー。 */
    COM_UTIL_TRACE_LEVEL_ERROR    = 1, /**< エラー。 */
    COM_UTIL_TRACE_LEVEL_WARNING  = 2, /**< 警告。 */
    COM_UTIL_TRACE_LEVEL_INFO     = 3, /**< 情報。 */
    COM_UTIL_TRACE_LEVEL_VERBOSE  = 4, /**< 詳細な診断情報。 */
    COM_UTIL_TRACE_LEVEL_DEBUG    = 5, /**< 最も詳細な診断情報。 */
    COM_UTIL_TRACE_LEVEL_NONE     = 6  /**< 出力しない。 */
} com_util_trace_level_t;

/**
 *  @enum           com_util_tracer_state_t
 *  @brief          tracer handle のライフサイクル状態。
 */
typedef enum com_util_tracer_state_t
{
    COM_UTIL_TRACER_STATE_STOPPED  = 0, /**< 作成済みで停止中。 */
    COM_UTIL_TRACER_STATE_STARTED  = 1, /**< 作成済みで実行中。 */
    COM_UTIL_TRACER_STATE_DISPOSED = 2  /**< 利用不可または解放済み。 */
} com_util_tracer_state_t;

/* ===== デフォルトトレースレベル ===== */

/**
 *  @def            COM_UTIL_TRACER_DEFAULT_OS_LEVEL
 *  @brief          com_util_tracer_create() が設定する OS トレース (ETW / syslog) のデフォルトレベル。
 *
 *  ユーザーが com_util_tracer_set_os_level() で変更するまで有効な初期値です。
 */
#define COM_UTIL_TRACER_DEFAULT_OS_LEVEL   COM_UTIL_TRACE_LEVEL_INFO

/**
 *  @def            COM_UTIL_TRACER_DEFAULT_FILE_LEVEL
 *  @brief          com_util_tracer_create() が設定するファイルトレースのデフォルトレベル。
 *
 *  ユーザーが com_util_tracer_set_file_level() で変更するまで有効な初期値です。
 */
#define COM_UTIL_TRACER_DEFAULT_FILE_LEVEL COM_UTIL_TRACE_LEVEL_ERROR

/**
 *  @def            COM_UTIL_TRACER_DEFAULT_STDERR_LEVEL
 *  @brief          com_util_tracer_create() が設定する stderr トレースのデフォルトレベル。
 *
 *  ユーザーが com_util_tracer_set_stderr_level() で変更するまで有効な初期値です。\n
 *  デフォルトは COM_UTIL_TRACE_LEVEL_NONE (無効) です。
 */
#define COM_UTIL_TRACER_DEFAULT_STDERR_LEVEL COM_UTIL_TRACE_LEVEL_NONE

/* ===== 不透明ハンドル型 ===== */

/** トレースプロバイダハンドル (不透明型)。 */
typedef struct com_util_tracer com_util_tracer_t;

/* ===== API 関数 ===== */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *******************************************************************************
     *  @brief          トレースプロバイダを初期化する。
     *
     *  自プロセスの実行ファイル名をデフォルト識別名として初期化します
     *  (例: Linux @c /usr/bin/myapp → @c "myapp",
     *  Windows @c C:\\bin\\myapp.exe → @c "myapp.exe")。\n
     *  プロセス名の取得に失敗した場合は @c "unknown" を使用します。\n
     *  Linux 環境では syslog を LOG_USER facility で初期化します。\n
     *  Windows 環境ではライブラリ内蔵の ETW デフォルトプロバイダ
     *  (@c COM_UTIL_TRACER_DEFAULT_PROVIDER_NAME) を使用します。\n
     *  識別名を変更するには com_util_tracer_set_name を呼び出してください。
     *
     *  @return         成功時: ハンドル (stopped 状態)。失敗時: NULL。
     *
     *  @post           戻り値のハンドルは stopped 状態です。
     *                  出力関数を使用するには com_util_tracer_start を呼び出してください。\n
     *                  stopped 状態では設定関数 (com_util_tracer_set_name, com_util_tracer_set_os_level,
     *                  com_util_tracer_set_file_level, com_util_tracer_set_stderr_level) をスレッド安全に使用できます。
     *
     *  @par            使用例
     *  @code{.c}
       com_util_tracer_t *tracer = com_util_tracer_create();
       com_util_tracer_set_name(tracer, "myapp", 0);
       com_util_tracer_start(tracer);
       com_util_tracer_write(tracer, COM_UTIL_TRACE_LEVEL_INFO, NULL, "application started");
       com_util_tracer_stop(tracer);
       com_util_tracer_dispose(tracer);
     *  @endcode
     *
     *  @par            スレッド セーフティ
     *  本関数はスレッドセーフです。\n
     *  複数スレッドから独立したハンドルを取得するために並行して呼び出すことができます。
     *******************************************************************************
     */
    COM_UTIL_EXPORT com_util_tracer_t *COM_UTIL_API
        com_util_tracer_create(void);

    /**
     *******************************************************************************
     *  @brief          トレースプロバイダを開始する。
     *
     *  ハンドルを実行中 (started) 状態に遷移させます。\n
     *  started 状態では出力関数 (com_util_tracer_write 等) が有効になり、
     *  設定関数 (com_util_tracer_set_name, com_util_tracer_set_os_level, com_util_tracer_set_file_level,
     *  com_util_tracer_set_stderr_level) は使用できなくなります (-1 を返します)。\n
     *  すでに started 状態の場合は何もせず 0 を返します (冪等)。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @return         成功 0 / 失敗 -1。
     *
     *  @par            スレッド セーフティ
     *  本関数はスレッドセーフです。\n
     *  内部で排他制御を行います。
     *
     *  @warning        handle が NULL の場合は -1 を返します。
     *
     *  @see            com_util_tracer_stop
     *******************************************************************************
     */
    COM_UTIL_EXPORT int COM_UTIL_API
        com_util_tracer_start(com_util_tracer_t *handle);

    /**
     *******************************************************************************
     *  @brief          トレースプロバイダを停止する。
     *
     *  ハンドルを停止中 (stopped) 状態に遷移させます。\n
     *  stopped 状態では出力関数 (com_util_tracer_write 等) は -1 を返し、
     *  設定関数 (com_util_tracer_set_name, com_util_tracer_set_os_level, com_util_tracer_set_file_level,
     *  com_util_tracer_set_stderr_level) がスレッド安全に使用できるようになります。\n
     *  すでに stopped 状態の場合は何もせず 0 を返します (冪等)。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @return         成功 0 / 失敗 -1。
     *
     *  @par            スレッド セーフティ
     *  本関数はスレッドセーフです。\n
     *  内部で排他制御を行います。
     *
     *  @warning        handle が NULL の場合は -1 を返します。
     *
     *  @see            com_util_tracer_start
     *******************************************************************************
     */
    COM_UTIL_EXPORT int COM_UTIL_API
        com_util_tracer_stop(com_util_tracer_t *handle);

    /**
     *******************************************************************************
     *  @brief          tracer handle の現在状態を取得する。
     *
     *  create 直後および stop 後は stopped、start 後は started を返します。\n
     *  handle が NULL、解放済み、または shutdown 中で利用できない場合は disposed を返します。\n
     *  dispose 実行後のポインタ再利用は未定義動作のため、disposed 判定には NULL または
     *  呼び出し側が保持するセッション状態を使用してください。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。NULL 可。
     *  @return         現在の状態 (com_util_tracer_state_t)。
     *******************************************************************************
     */
    COM_UTIL_EXPORT com_util_tracer_state_t COM_UTIL_API
        com_util_tracer_get_state(com_util_tracer_t *handle);

    /**
     *******************************************************************************
     *  @brief          トレースメッセージを書き込む低レベル関数。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @param[in]      level    トレースレベル (com_util_trace_level_t)。
     *  @param[in]      timestamp  使用する実時刻。NULL の場合は API 内部で現在時刻を取得。
     *                             不正な明示タイムスタンプが渡された場合も現在時刻で出力を継続し、
     *                             戻り値は -1 を返します。
     *  @param[in]      message  null 終端 UTF-8 文字列。
     *  @return         成功 0 / 失敗 -1。
     *******************************************************************************
     */
    COM_UTIL_EXPORT int COM_UTIL_API
        _com_util_tracer_write(com_util_tracer_t *handle, com_util_trace_level_t level,
                               const com_util_realtime_timestamp_t *timestamp, const char *message);

    /**
     *******************************************************************************
     *  @brief          printf 形式でトレースメッセージを書き込む低レベル関数。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @param[in]      level    トレースレベル (com_util_trace_level_t)。
     *  @param[in]      timestamp  使用する実時刻。NULL の場合は API 内部で現在時刻を取得。
     *                             不正な明示タイムスタンプが渡された場合も現在時刻で出力を継続し、
     *                             戻り値は -1 を返します。
     *  @param[in]      format   printf 形式のフォーマット文字列。
     *  @param[in]      ...      フォーマット文字列に対応する可変長引数。
     *  @return         成功 0 / 失敗 -1。
     *******************************************************************************
     */
    COM_UTIL_EXPORT int COM_UTIL_API
        _com_util_tracer_writef(com_util_tracer_t *handle, com_util_trace_level_t level,
                                const com_util_realtime_timestamp_t *timestamp, const char *format, ...);

    /**
     *******************************************************************************
     *  @brief          バイナリデータを HEX テキスト形式でトレースに書き込む低レベル関数。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @param[in]      level    トレースレベル (com_util_trace_level_t)。
     *  @param[in]      timestamp  使用する実時刻。NULL の場合は API 内部で現在時刻を取得。
     *                             不正な明示タイムスタンプが渡された場合も現在時刻で出力を継続し、
     *                             戻り値は -1 を返します。
     *  @param[in]      data     バイナリデータへのポインタ。
     *  @param[in]      size     バイナリデータのバイト数。
     *  @param[in]      message  HEX データの手前に付与するラベル文字列。NULL 可。
     *  @return         成功 0 / 失敗 -1。
     *******************************************************************************
     */
    COM_UTIL_EXPORT int COM_UTIL_API
        _com_util_tracer_write_hex(com_util_tracer_t *handle, com_util_trace_level_t level,
                                   const com_util_realtime_timestamp_t *timestamp,
                                   const void *data, size_t size, const char *message);

    /**
     *******************************************************************************
     *  @brief          バイナリデータを HEX テキスト形式でトレースに書き込む低レベル関数 (printf 形式ラベル)。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @param[in]      level    トレースレベル (com_util_trace_level_t)。
     *  @param[in]      timestamp  使用する実時刻。NULL の場合は API 内部で現在時刻を取得。
     *                             不正な明示タイムスタンプが渡された場合も現在時刻で出力を継続し、
     *                             戻り値は -1 を返します。
     *  @param[in]      data     バイナリデータへのポインタ。
     *  @param[in]      size     バイナリデータのバイト数。
     *  @param[in]      format   printf 形式のフォーマット文字列 (ラベル)。NULL 可。
     *  @param[in]      ...      フォーマット文字列に対応する可変長引数。
     *  @return         成功 0 / 失敗 -1。
     *******************************************************************************
     */
    COM_UTIL_EXPORT int COM_UTIL_API
        _com_util_tracer_write_hexf(com_util_tracer_t *handle, com_util_trace_level_t level,
                                    const com_util_realtime_timestamp_t *timestamp,
                                    const void *data, size_t size, const char *format, ...);

    /**
     *******************************************************************************
     *  @brief          トレースプロバイダの識別名と識別番号を設定する。
     *
     *  @param[in]      handle      com_util_tracer_create の戻り値。
     *  @param[in]      name        ベース識別名。NULL で自プロセス名を使用。
     *  @param[in]      identifier  アプリケーション管理識別番号 (0 以上)。
     *  @return         成功 0 / 失敗 -1。
     *******************************************************************************
     */
    COM_UTIL_EXPORT int COM_UTIL_API
        com_util_tracer_set_name(com_util_tracer_t *handle, const char *name, int64_t identifier);

    /**
     *******************************************************************************
     *  @brief          OS トレースの現在のスレッショルドレベルを取得する。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @return         現在のスレッショルドレベル。handle が NULL 時は COM_UTIL_TRACE_LEVEL_NONE。
     *******************************************************************************
     */
    COM_UTIL_EXPORT com_util_trace_level_t COM_UTIL_API
        com_util_tracer_get_os_level(com_util_tracer_t *handle);

    /**
     *******************************************************************************
     *  @brief          OS トレースのスレッショルドレベルを設定する。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @param[in]      level    新しいスレッショルドレベル (com_util_trace_level_t)。
     *  @return         成功 0 / 失敗 -1。
     *******************************************************************************
     */
    COM_UTIL_EXPORT int COM_UTIL_API
        com_util_tracer_set_os_level(com_util_tracer_t *handle, com_util_trace_level_t level);

    /**
     *******************************************************************************
     *  @brief          ファイルトレースの現在のスレッショルドレベルを取得する。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @return         現在のスレッショルドレベル。handle が NULL 時は COM_UTIL_TRACE_LEVEL_NONE。
     *******************************************************************************
     */
    COM_UTIL_EXPORT com_util_trace_level_t COM_UTIL_API
        com_util_tracer_get_file_level(com_util_tracer_t *handle);

    /**
     *******************************************************************************
     *  @brief          ファイルトレースの出力先と設定を変更する。
     *
     *  @param[in]      handle       com_util_tracer_create の戻り値。
     *  @param[in]      path         出力ファイルパス。NULL でファイルトレースを無効化。
     *  @param[in]      level        ファイルトレースのスレッショルドレベル。
     *  @param[in]      max_bytes    1 ファイルあたりの最大バイト数。0 で既定値を使用。
     *  @param[in]      generations  保持する旧世代数。0 以下で既定値を使用。
     *  @return         成功 0 / 失敗 -1。
     *******************************************************************************
     */
    COM_UTIL_EXPORT int COM_UTIL_API
        com_util_tracer_set_file_level(com_util_tracer_t *handle, const char *path,
                             com_util_trace_level_t level, size_t max_bytes, int generations);

    /**
     *******************************************************************************
     *  @brief          stderr トレースの現在のスレッショルドレベルを取得する。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @return         現在のスレッショルドレベル。handle が NULL 時は COM_UTIL_TRACE_LEVEL_NONE。
     *******************************************************************************
     */
    COM_UTIL_EXPORT com_util_trace_level_t COM_UTIL_API
        com_util_tracer_get_stderr_level(com_util_tracer_t *handle);

    /**
     *******************************************************************************
     *  @brief          stderr トレースのスレッショルドレベルを設定する。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @param[in]      level    新しいスレッショルドレベル (com_util_trace_level_t)。
     *  @return         成功 0 / 失敗 -1。
     *******************************************************************************
     */
    COM_UTIL_EXPORT int COM_UTIL_API
        com_util_tracer_set_stderr_level(com_util_tracer_t *handle, com_util_trace_level_t level);

    /**
     *******************************************************************************
     *  @brief          トレースプロバイダを終了し、リソースを解放する。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。NULL は無視。
     *******************************************************************************
     */
    COM_UTIL_EXPORT void COM_UTIL_API
        com_util_tracer_dispose(com_util_tracer_t *handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* ===== ソース位置自動付与マクロ ===== */

/**
 *  @def            _com_util_tracer_basename(f)
 *  @brief          ファイルパスからベースネームを取り出す内部ヘルパーマクロ。
 *  @internal
 */
#define _com_util_tracer_basename(f) \
    (strrchr((f), PLATFORM_PATH_SEP_CHR) ? strrchr((f), PLATFORM_PATH_SEP_CHR) + 1 : \
     (strrchr((f), '\\') ? strrchr((f), '\\') + 1 : (f)))

/**
 *  @def            com_util_tracer_write(handle, level, timestamp, message)
 *  @brief          ソースファイル名と行番号を自動付与する com_util_tracer_write マクロ。
 */
#define com_util_tracer_write(handle, level, timestamp, message) \
    ((message) != NULL ? \
         _com_util_tracer_writef((handle), (level), (timestamp), "[%s:%d] %s", \
                                 _com_util_tracer_basename(__FILE__), __LINE__, (message)) : \
         _com_util_tracer_write((handle), (level), (timestamp), (message)))

/**
 *  @def            com_util_tracer_writef(handle, level, timestamp, fmt, ...)
 *  @brief          ソースファイル名と行番号を自動付与する com_util_tracer_writef マクロ。
 */
#define com_util_tracer_writef(handle, level, timestamp, fmt, ...) \
    _com_util_tracer_writef((handle), (level), (timestamp), "[%s:%d] " fmt, \
                            _com_util_tracer_basename(__FILE__), __LINE__, ##__VA_ARGS__)

/**
 *  @def            com_util_tracer_write_hex(handle, level, timestamp, data, size, message)
 *  @brief          ソースファイル名と行番号を自動付与する com_util_tracer_write_hex マクロ。
 */
#define com_util_tracer_write_hex(handle, level, timestamp, data, size, message) \
    _com_util_tracer_write_hexf((handle), (level), (timestamp), (data), (size), "[%s:%d]%s%s", \
                                _com_util_tracer_basename(__FILE__), __LINE__, \
                                ((message) != NULL && (message)[0] != '\0') ? " " : "", \
                                (message) != NULL ? (message) : "")

/**
 *  @def            com_util_tracer_write_hexf(handle, level, timestamp, data, size, fmt, ...)
 *  @brief          ソースファイル名と行番号を自動付与する com_util_tracer_write_hexf マクロ。
 */
#define com_util_tracer_write_hexf(handle, level, timestamp, data, size, fmt, ...) \
    _com_util_tracer_write_hexf((handle), (level), (timestamp), (data), (size), "[%s:%d] " fmt, \
                                _com_util_tracer_basename(__FILE__), __LINE__, ##__VA_ARGS__)

#endif /* COM_UTIL_TRACER_H */
