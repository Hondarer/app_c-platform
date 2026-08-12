/**
 *******************************************************************************
 *  @file           tracer.h
 *  @brief          クロスプラットフォームのトレーシング API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/03/05
 *  @version        1.0.0
 *
 *  Windows (ETW) と Linux (syslog) の差異を抽象化し、
 *  共通のトレース レベルとインターフェースを提供します。\n
 *  内部で `com_util/trace/etw.h` (Windows) または `com_util/trace/syslog.h` (Linux) を
 *  使用します。
 *
 *  @par            アーキテクチャー
    @code
   Application
         |
         v  tracer.h (共通 API)
         |
   +-----+-----+------+--------+
   |           |      |        |
   ETW        syslog File    stderr
   (Windows)  (Linux) (両OS)  (両OS)
    @endcode
 *
 *  @par            使用例 (共通)
    @code{.c}
   #include <com_util/trace/tracer.h>

   com_util_tracer *tracer = com_util_tracer_create();
   com_util_tracer_set_name(tracer, "myapp", 0);
   com_util_tracer_start(tracer);
   com_util_tracer_write(tracer, COM_UTIL_TRACE_LEVEL_INFO, NULL, "application started");
   com_util_tracer_stop(tracer);
   com_util_tracer_dispose(tracer);
    @endcode
 *
 *  @par            使用例 (設定変更)
    @code{.c}
   com_util_tracer *tracer = com_util_tracer_create();
   com_util_tracer_set_name(tracer, "myapp", 0);
   com_util_tracer_set_os_level(tracer, COM_UTIL_TRACE_LEVEL_VERBOSE);
   com_util_tracer_start(tracer);
   com_util_tracer_write(tracer, COM_UTIL_TRACE_LEVEL_INFO, NULL, "running as myapp");
   com_util_tracer_stop(tracer);
   com_util_tracer_set_name(tracer, "myapp", 1); // "myapp_1" として再開
   com_util_tracer_start(tracer);
   com_util_tracer_write(tracer, COM_UTIL_TRACE_LEVEL_INFO, NULL, "running as myapp_1");
   com_util_tracer_stop(tracer);
   com_util_tracer_dispose(tracer);
    @endcode
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_TRACER_H
#define COM_UTIL_TRACER_H

/* size_t (com_util_tracer_write_hex / com_util_tracer_write_hexf で使用) */
#include <stddef.h>
/* int64_t (com_util_tracer_set_name で使用) */
#include <inttypes.h>
#include <com_util/base/platform.h>
#include <com_util/base/result.h>
#include <com_util/clock/clock.h>
#include <com_util/crt/path.h>
#include <com_util/com_util_export.h>

/* 内部で使用するプラットフォーム固有ヘッダー */
#if defined(PLATFORM_LINUX)
    #include <com_util/trace/syslog.h>
#elif defined(PLATFORM_WINDOWS)
    #include <com_util/trace/etw.h>
#endif /* PLATFORM_ */

/**
 *  @ingroup        COM_UTIL_TRACE
 *  @{
 */

/* ===== デフォルト プロバイダー定義 (Windows) ===== */

#if defined(PLATFORM_WINDOWS)

    /**
     *  @brief          com_util が使用するデフォルトの OS トレース識別子 (Windows) です。
     *
     *  com_util_tracer_create が使用する ETW プロバイダー名と、
     *  EventLog の共通イベント ソース名 (@ref eventlog.h) を兼ねます。\n
     *  ETW と EventLog で同一の識別子を共用します。
     */
    #define COM_UTIL_TRACER_DEFAULT_PROVIDER_NAME "com_util.tracer"

    /**
     *  @brief          デフォルト ETW プロバイダーの GUID (TraceLogging タプル形式) です。
     *
     *  TRACELOGGING_DEFINE_PROVIDER で使用する形式です。
     */
    #define COM_UTIL_TRACER_DEFAULT_PROVIDER_GUID \
        (0xc3a7b5d1, 0x4e2f, 0x4a89, 0x96, 0xc8, 0xd7, 0xe9, 0xf1, 0xa2, 0xb3, 0xc4)

    /**
     *  @brief          デフォルト ETW プロバイダーの GUID (文字列形式) です。
     *
     *  com_util_etw_session_start に渡す場合など、文字列形式の GUID が
     *  必要な場面で使用します。
     */
    #define COM_UTIL_TRACER_DEFAULT_PROVIDER_GUID_STR "c3a7b5d1-4e2f-4a89-96c8-d7e9f1a2b3c4"

#endif /* PLATFORM_WINDOWS */

/* ===== メッセージ長制限 ===== */

/**
 *  @brief          com_util_tracer_write が受け付けるメッセージの最大バイト数 (null 終端含む) です。
 *
 *  ETW (約 65,000 バイト) と syslog (RFC 3164: 1,024 バイト) の
 *  推奨上限のうち小さい方を採用し、クロスプラットフォームでの
 *  安全な転送を保証します。\n
 *  本文の最大長は `COM_UTIL_TRACER_MESSAGE_MAX_BYTES` @c - @c 1 (= 1,023) バイトです。
 */
#define COM_UTIL_TRACER_MESSAGE_MAX_BYTES 1024

/**
 *  @brief          com_util_tracer_write_hex がラベルなしで HEX 出力できるバイナリ データの最大バイト数です。
 *
 *  1 バイトあたり 3 文字 (HH + スペース) を消費し、最終バイトは 2 文字です。\n
 *  ラベル (@p message) を指定した場合はラベル長 + セパレータ (": ") 分だけ
 *  出力可能なバイナリ データ量が減少します。\n
 *  データがこの上限を超える場合は切り詰めが行われ、
 *  末尾に `"..."` が付与されます。
 */
#define COM_UTIL_TRACER_HEX_MAX_DATA_BYTES 341

/* ===== 共通トレース レベル ===== */

/**
 *  @enum           com_util_trace_level
 *  @brief          アプリケーション共通トレース レベルです。
 *
 *  OS 非依存のトレース レベルを定義します。重大度は上から下へ低下します。\n
 *  内部で ETW Level (1-5) および syslog severity へマッピングされます。\n
 *  COM_UTIL_TRACE_LEVEL_DEBUG は ETW / syslog では COM_UTIL_TRACE_LEVEL_VERBOSE と同じ詳細度で扱われます。
 *
 *  | com_util_trace_level          | ETW Level         | syslog severity |
 *  | ----------------------------- | ----------------- | --------------- |
 *  | COM_UTIL_TRACE_LEVEL_CRITICAL | Critical (1)      | LOG_CRIT (2)    |
 *  | COM_UTIL_TRACE_LEVEL_ERROR    | Error (2)         | LOG_ERR (3)     |
 *  | COM_UTIL_TRACE_LEVEL_WARNING  | Warning (3)       | LOG_WARNING (4) |
 *  | COM_UTIL_TRACE_LEVEL_INFO     | Informational (4) | LOG_INFO (6)    |
 *  | COM_UTIL_TRACE_LEVEL_VERBOSE  | Verbose (5)       | LOG_DEBUG (7)   |
 *  | COM_UTIL_TRACE_LEVEL_DEBUG    | Verbose (5)       | LOG_DEBUG (7)   |
 */
typedef enum com_util_trace_level
{
    COM_UTIL_TRACE_LEVEL_CRITICAL = 0, /**< 致命的エラー。 */
    COM_UTIL_TRACE_LEVEL_ERROR = 1,    /**< エラー。 */
    COM_UTIL_TRACE_LEVEL_WARNING = 2,  /**< 警告。 */
    COM_UTIL_TRACE_LEVEL_INFO = 3,     /**< 情報。 */
    COM_UTIL_TRACE_LEVEL_VERBOSE = 4,  /**< 詳細な診断情報。 */
    COM_UTIL_TRACE_LEVEL_DEBUG = 5,    /**< 最も詳細な診断情報。 */
    COM_UTIL_TRACE_LEVEL_NONE = 6      /**< 出力しない。 */
} com_util_trace_level;

/**
 *  @enum           com_util_tracer_state
 *  @brief          tracer handle のライフサイクル状態です。
 */
typedef enum com_util_tracer_state
{
    COM_UTIL_TRACER_STATE_STOPPED = 0, /**< 作成済みで停止中。 */
    COM_UTIL_TRACER_STATE_STARTED = 1, /**< 作成済みで実行中。 */
    COM_UTIL_TRACER_STATE_DISPOSED = 2 /**< 利用不可または解放済み。 */
} com_util_tracer_state;

/* ===== デフォルト トレース レベル ===== */

/**
 *  @brief          com_util_tracer_create() が設定する OS トレース (EventLog / syslog) のデフォルト レベルです。
 *
 *  OS トレースは Windows ではイベント ログ (EventLog)、Linux では syslog を指します。\n
 *  運用者が参照する OS ネイティブの運用ログであり、ユーザーが
 *  com_util_tracer_set_os_level() で変更するまで有効な初期値です。\n
 *  デフォルトは COM_UTIL_TRACE_LEVEL_NONE (無効) です。
 */
#define COM_UTIL_TRACER_DEFAULT_OS_LEVEL COM_UTIL_TRACE_LEVEL_NONE

/**
 *  @brief          com_util_tracer_create() が設定する ETW トレースのデフォルト レベルです。
 *
 *  ETW (Event Tracing for Windows) は開発者向けの低オーバーヘッド診断チャネルであり、
 *  OS トレース (EventLog) とは独立した軸として制御します。\n
 *  ETW イベントはコンシューマー (etw-viewer など) が購読したときのみ実体化されるため、
 *  デフォルトで有効 (COM_UTIL_TRACE_LEVEL_VERBOSE) としています。\n
 *  ユーザーが com_util_tracer_set_etw_level() で変更するまで有効な初期値です。\n
 *  本定義は Windows でのみ意味を持ちます。Linux では ETW は存在せず、
 *  com_util_tracer_set_etw_level() / com_util_tracer_get_etw_level() は何もしません。
 */
#define COM_UTIL_TRACER_DEFAULT_ETW_LEVEL COM_UTIL_TRACE_LEVEL_VERBOSE

/**
 *  @brief          com_util_tracer_create() が設定するファイル トレースのデフォルト レベルです。
 *
 *  ユーザーが com_util_tracer_set_file_level() で変更するまで有効な初期値です。\n
 *  ファイル トレースはデフォルトで有効であり、com_util_tracer_set_file_level() を呼び出さない場合、
 *  com_util_tracer_start() 時にデフォルト パス
 *  (実行ファイルのディレクトリ配下の `log/{ファイル名}.log`。
 *  ファイル名のデフォルトはプロセス名) へ出力されます。
 */
#define COM_UTIL_TRACER_DEFAULT_FILE_LEVEL COM_UTIL_TRACE_LEVEL_INFO

/**
 *  @brief          com_util_tracer_create() が設定する stderr トレースのデフォルト レベルです。
 *
 *  ユーザーが com_util_tracer_set_stderr_level() で変更するまで有効な初期値です。\n
 *  デフォルトは COM_UTIL_TRACE_LEVEL_NONE (無効) です。
 */
#define COM_UTIL_TRACER_DEFAULT_STDERR_LEVEL COM_UTIL_TRACE_LEVEL_NONE

/* ===== 不透明ハンドル型 ===== */

/** トレース プロバイダー ハンドル (不透明型)。 */
typedef struct com_util_tracer com_util_tracer;

/* ===== フック (コールバック) ===== */

/**
 *  @brief  トレース フック エントリ (不透明型) です。
 *
 *  com_util_tracer_set_hook が返す不透明ハンドル。\n
 *  com_util_tracer_remove_hook および com_util_tracer_call_next_hook に渡して使用します。
 */
typedef struct com_util_tracer_hook_entry com_util_tracer_hook_entry;

/**
 *  @brief  トレース フックのコールバック関数型です。
 *
 *  @param[in]  prev      チェーン継続に使う前エントリ。com_util_tracer_call_next_hook に渡します。
 *  @param[in]  handle    trace を行った tracer ハンドル。
 *  @param[in]  level     trace レベル (COM_UTIL_TRACE_LEVEL_NONE を含む全レベル)。
 *  @param[in]  timestamp 解決済みタイムスタンプ (常に有効)。
 *  @param[in]  message   解決済みメッセージ文字列。
 *  @param[in]  context   com_util_tracer_set_hook で渡したコンテキスト。
 *
 *  @par        チェーン例
    @code{.c}
    void my_hook(com_util_tracer_hook_entry *prev,
                 com_util_tracer *handle,
                 com_util_trace_level level,
                 const com_util_timespec *timestamp,
                 const char *message, void *context)
    {
        // 独自処理
        printf("hook: %s\n", message);
        // 前のフックへ継続 (省略すると以降のチェーンは呼ばれない)
        com_util_tracer_call_next_hook(prev, handle, level, timestamp, message);
    }
    @endcode
 *
 *  @par            スレッド セーフ
 *  コールバックは複数スレッドから同時に呼び出される可能性があります。\n
 *  コールバックの実装者は再入性を確保してください。
 */
typedef void (*com_util_tracer_hook_fn)(com_util_tracer_hook_entry *prev, com_util_tracer *handle,
                                        com_util_trace_level level, const com_util_timespec *timestamp,
                                        const char *message, void *context);

/* ===== API 関数 ===== */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          トレース プロバイダーを初期化します。
     *
     *  自プロセスの実行ファイル名をデフォルト識別名として初期化します
     *  (例: Linux `/usr/bin/myapp` → `"myapp"`,
     *  Windows `C:\bin\myapp.exe` → `"myapp.exe"`)。\n
     *  プロセス名の取得に失敗した場合は `"unknown"` を使用します。\n
     *  Linux 環境では syslog を LOG_USER facility で初期化します。\n
     *  Windows 環境ではライブラリ内蔵の ETW デフォルト プロバイダー
     *  (`COM_UTIL_TRACER_DEFAULT_PROVIDER_NAME`) を使用します。\n
     *  識別名を変更するには com_util_tracer_set_name を呼び出してください。
     *
     *  デフォルトの出力先はファイル トレースのみです
     *  (OS トレースと stderr トレースのデフォルト レベルは COM_UTIL_TRACE_LEVEL_NONE)。\n
     *  ファイル トレースの出力先はデフォルトで実行ファイルのディレクトリ配下の
     *  `log/{ファイル名}.log` であり、占有モード、最大
     *  COM_UTIL_TRACE_FILE_SINK_DEFAULT_MAX_BYTES バイト、
     *  COM_UTIL_TRACE_FILE_SINK_DEFAULT_GENERATIONS 世代で運用されます。
     *  ファイル名のデフォルトはプロセス名 (実行ファイルのベース名。Windows は末尾の `.exe` を除く) です。\n
     *  パスとパラメーターは com_util_tracer_set_file_level で、
     *  ファイル名とファイル識別は com_util_tracer_set_file_name で変更できます。
     *
     *  識別名 (インスタンス名とインスタンス識別) とトレース ファイル名 (ファイル名とファイル識別) は
     *  独立して管理されます。com_util_tracer_set_name はトレース ファイル名に影響しません。
     *
     *  @return         成功時は stopped 状態のハンドルを返します。メモリ確保または同期オブジェクトの
     *                  初期化に失敗した場合は NULL を返します。
     *
     *  @post           戻り値のハンドルは stopped 状態です。
     *                  出力関数を使用するには com_util_tracer_start を呼び出してください。\n
     *                  識別子・ファイル名・フックの設定関数 (com_util_tracer_set_name,
     *                  com_util_tracer_set_file_name, com_util_tracer_set_hook, com_util_tracer_remove_hook) は
     *                  stopped 状態でのみスレッド安全に使用できます。\n
     *                  レベル設定関数 (com_util_tracer_set_os_level, com_util_tracer_set_etw_level,
     *                  com_util_tracer_set_file_level, com_util_tracer_set_stderr_level) は
     *                  stopped / started のどちらでも使用できます。
     *
     *  @par            使用例
        @code{.c}
       com_util_tracer *tracer = com_util_tracer_create();
       com_util_tracer_set_name(tracer, "myapp", 0);
       com_util_tracer_start(tracer);
       com_util_tracer_write(tracer, COM_UTIL_TRACE_LEVEL_INFO, NULL, "application started");
       com_util_tracer_stop(tracer);
       com_util_tracer_dispose(tracer);
        @endcode
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  複数スレッドから独立したハンドルを取得するために並行して呼び出すことができます。
     */
    COM_UTIL_EXPORT com_util_tracer *COM_UTIL_API com_util_tracer_create(void);

    /**
     *  @brief          トレース プロバイダーを開始します。
     *
     *  ハンドルを実行中 (started) 状態に遷移させます。\n
     *  started 状態では出力関数 (com_util_tracer_write 等) が有効になります。\n
     *  レベル設定関数 (com_util_tracer_set_os_level, com_util_tracer_set_etw_level,
     *  com_util_tracer_set_file_level, com_util_tracer_set_stderr_level) は started 状態でも使用でき、
     *  停止せずにしきい値レベルを変更できます。\n
     *  識別子・ファイル名・フックの設定関数 (com_util_tracer_set_name, com_util_tracer_set_file_name,
     *  com_util_tracer_set_hook, com_util_tracer_remove_hook) は started 状態では使用できません (@ref COM_UTIL_ERR_UNKNOWN / NULL を返します)。\n
     *  すでに started 状態の場合は何もせず @ref COM_UTIL_OK を返します (べき等)。
     *
     *  ファイル トレースのレベルが COM_UTIL_TRACE_LEVEL_NONE 以外の場合、
     *  本関数の呼び出し時点の設定 (出力ファイル パス、ファイル名、ファイル識別) で
     *  トレース ファイルを開きます。\n
     *  出力ファイル パスの妥当性 (オープン可否) は本関数の戻り値で報告されます。\n
     *  トレース ファイルを開けなかった場合も started 状態へは遷移し、
     *  ファイル以外のトレース出力 (OS / stderr / フック) は継続したうえで @ref COM_UTIL_ERR_UNKNOWN を返します。
     *  この場合、com_util_tracer_stop 後に再度本関数を呼び出すとオープンを再試行します。
     *
     *  同一プロセス内の複数の tracer が同一パスのトレース ファイルを開いた場合は、
     *  プロセス内で同一ファイルへの書き込みが調停されるため、占有モードでも併用できます
     *  (詳細は com_util_trace_file_sink_create を参照)。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部で排他制御を行います。
     *
     *  @warning        handle が NULL の場合は @ref COM_UTIL_ERR_UNKNOWN を返します。
     *  @warning        別プロセスとの間では占有モードの排他が働くため、同一実行ファイルを複数プロセス
     *                  起動するとデフォルト パスのオープンが 2 プロセス目以降で失敗する場合があります
     *                  (Windows)。com_util_tracer_set_file_name のファイル識別、または
     *                  com_util_tracer_set_file_level の明示パスでプロセスごとにファイルを分けてください。
     *
     *  @see            com_util_tracer_stop
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_tracer_start(com_util_tracer *handle);

    /**
     *  @brief          トレース プロバイダーを停止します。
     *
     *  ハンドルを停止中 (stopped) 状態に遷移させます。\n
     *  stopped 状態では出力関数 (com_util_tracer_write 等) は @ref COM_UTIL_ERR_UNKNOWN を返し、
     *  識別子・ファイル名・フックの設定関数 (com_util_tracer_set_name, com_util_tracer_set_file_name,
     *  com_util_tracer_set_hook, com_util_tracer_remove_hook) がスレッド安全に使用できるようになります。\n
     *  レベル設定関数 (com_util_tracer_set_os_level 等) は stopped / started のどちらでも使用できます。\n
     *  ファイル トレースが有効な場合、開いていたトレース ファイルを閉じます。
     *  ファイル トレースの設定は保持され、次回の com_util_tracer_start で改めてファイルを開きます。\n
     *  すでに stopped 状態の場合は何もせず @ref COM_UTIL_OK を返します (べき等)。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部で排他制御を行います。
     *
     *  @warning        handle が NULL の場合は @ref COM_UTIL_ERR_UNKNOWN を返します。
     *
     *  @see            com_util_tracer_start
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_tracer_stop(com_util_tracer *handle);

    /**
     *  @brief          tracer handle の現在状態を取得します。
     *
     *  create 直後および stop 後は stopped、start 後は started を返します。\n
     *  handle が NULL、解放済み、または shutdown 中で利用できない場合は disposed を返します。\n
     *  dispose 実行後のポインター再利用は未定義動作のため、disposed 判定には NULL または
     *  呼び出し側が保持するセッション状態を使用してください。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。NULL 可。
     *  @return         現在の状態 (com_util_tracer_state)。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  返される状態は取得時点のスナップショットです。呼び出し直後に状態が変化する場合があります。
     */
    COM_UTIL_EXPORT com_util_tracer_state COM_UTIL_API com_util_tracer_get_state(com_util_tracer *handle);

    /**
     *  @brief          トレース メッセージを書き込む低レベル関数です。
     *
     *  @param[in]      handle     com_util_tracer_create の戻り値。
     *  @param[in]      level      トレース レベル (com_util_trace_level)。
     *  @param[in]      timestamp  使用する実時刻。NULL の場合は API 内部で現在時刻を取得。
     *                             不正な明示タイムスタンプが渡された場合も現在時刻で出力を継続し、
     *                             戻り値は @ref COM_UTIL_ERR_UNKNOWN を返します。
     *  @param[in]      message    null 終端 UTF-8 文字列。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部で共有ロックを取得して設定を参照し、複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API _com_util_tracer_write(com_util_tracer *handle, com_util_trace_level level,
                                                            const com_util_timespec *timestamp, const char *message);

    /**
     *  @brief          printf 形式でトレース メッセージを書き込む低レベル関数です。
     *
     *  @param[in]      handle     com_util_tracer_create の戻り値。
     *  @param[in]      level      トレース レベル (com_util_trace_level)。
     *  @param[in]      timestamp  使用する実時刻。NULL の場合は API 内部で現在時刻を取得。
     *                             不正な明示タイムスタンプが渡された場合も現在時刻で出力を継続し、
     *                             戻り値は @ref COM_UTIL_ERR_UNKNOWN を返します。
     *  @param[in]      format     printf 形式のフォーマット文字列。
     *  @param[in]      ...        フォーマット文字列に対応する可変長引数。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部で共有ロックを取得して設定を参照し、複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API _com_util_tracer_writef(com_util_tracer *handle, com_util_trace_level level,
                                                             const com_util_timespec *timestamp, const char *format,
                                                             ...);

    /**
     *  @brief          書式付きメッセージをトレースに書き込む低レベル関数 (`_com_util_tracer_writef` の `va_list` 版) です。
     *
     *  @param[in]      handle     com_util_tracer_create の戻り値。
     *  @param[in]      level      トレース レベル (com_util_trace_level)。
     *  @param[in]      timestamp  使用する実時刻。NULL の場合は API 内部で現在時刻を取得。
     *  @param[in]      format     printf 形式のフォーマット文字列。
     *  @param[in]      args       フォーマット文字列に対応する引数リスト。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部で共有ロックを取得して設定を参照し、複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API _com_util_tracer_vwritef(com_util_tracer *handle, com_util_trace_level level,
                                                              const com_util_timespec *timestamp, const char *format,
                                                              va_list args);

    /**
     *  @brief          バイナリ データを HEX テキスト形式でトレースに書き込む低レベル関数です。
     *
     *  @param[in]      handle     com_util_tracer_create の戻り値。
     *  @param[in]      level      トレース レベル (com_util_trace_level)。
     *  @param[in]      timestamp  使用する実時刻。NULL の場合は API 内部で現在時刻を取得。
     *                             不正な明示タイムスタンプが渡された場合も現在時刻で出力を継続し、
     *                             戻り値は @ref COM_UTIL_ERR_UNKNOWN を返します。
     *  @param[in]      data       バイナリ データへのポインター。
     *  @param[in]      size       バイナリ データのバイト数。
     *  @param[in]      message    HEX データの手前に付与するラベル文字列。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部で共有ロックを取得して設定を参照し、複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API _com_util_tracer_write_hex(com_util_tracer *handle, com_util_trace_level level,
                                                                const com_util_timespec *timestamp, const void *data,
                                                                size_t size, const char *message);

    /**
     *  @brief          バイナリ データを HEX テキスト形式でトレースに書き込む低レベル関数 (printf 形式ラベル) です。
     *
     *  @param[in]      handle     com_util_tracer_create の戻り値。
     *  @param[in]      level      トレース レベル (com_util_trace_level)。
     *  @param[in]      timestamp  使用する実時刻。NULL の場合は API 内部で現在時刻を取得。
     *                             不正な明示タイムスタンプが渡された場合も現在時刻で出力を継続し、
     *                             戻り値は @ref COM_UTIL_ERR_UNKNOWN を返します。
     *  @param[in]      data       バイナリ データへのポインター。
     *  @param[in]      size       バイナリ データのバイト数。
     *  @param[in]      format     printf 形式のフォーマット文字列 (ラベル)。NULL 可。
     *  @param[in]      ...        フォーマット文字列に対応する可変長引数。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部で共有ロックを取得して設定を参照し、複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API _com_util_tracer_write_hexf(com_util_tracer *handle, com_util_trace_level level,
                                                                 const com_util_timespec *timestamp, const void *data,
                                                                 size_t size, const char *format, ...);

    /**
     *  @brief          バイナリ データを HEX テキスト形式で書き込む低レベル関数 (`_com_util_tracer_write_hexf` の `va_list` 版) です。
     *
     *  @param[in]      handle     com_util_tracer_create の戻り値。
     *  @param[in]      level      トレース レベル (com_util_trace_level)。
     *  @param[in]      timestamp  使用する実時刻。NULL の場合は API 内部で現在時刻を取得。
     *  @param[in]      data       バイナリ データへのポインター。
     *  @param[in]      size       バイナリ データのバイト数。
     *  @param[in]      format     printf 形式のフォーマット文字列 (ラベル)。NULL 可。
     *  @param[in]      args       フォーマット文字列に対応する引数リスト。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部で共有ロックを取得して設定を参照し、複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API _com_util_tracer_vwrite_hexf(com_util_tracer *handle, com_util_trace_level level,
                                                                  const com_util_timespec *timestamp, const void *data,
                                                                  size_t size, const char *format, va_list args);

    /**
     *  @brief          トレース プロバイダーのインスタンス名とインスタンス識別を設定します。
     *
     *  OS トレース (syslog ident / EventLog のインスタンス名) と ETW (サービス名) で
     *  使用する識別名を設定します。
     *  識別名は `{name}` (identifier が 0 の場合) または `{name}_{identifier}` です。\n
     *  EventLog はソースが com_util 共通のため、本識別名を本文先頭に付与して
     *  インスタンスを判別可能にします。\n
     *  本関数はトレース ファイル名には影響しません。トレース ファイル名とファイル識別は
     *  com_util_tracer_set_file_name で独立して設定します。
     *
     *  @param[in]      handle      com_util_tracer_create の戻り値。
     *  @param[in]      name        インスタンス名。NULL で自プロセス名を使用。
     *  @param[in]      identifier  インスタンス識別番号 (0 以上)。0 でサフィックスなし。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、@ref COM_UTIL_ERR_OUT_OF_MEMORY 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  stopped 状態でのみ有効です。started 状態では @ref COM_UTIL_ERR_UNKNOWN を返します。
     *
     *  @see            com_util_tracer_get_name
     *  @see            com_util_tracer_get_identifier
     *  @see            com_util_tracer_set_file_name
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_tracer_set_name(com_util_tracer *handle, const char *name,
                                                              int64_t identifier);

    /**
     *  @brief          解決済みのインスタンス名 (識別番号サフィックス込み) を取得します。
     *
     *  OS トレース (syslog ident / EventLog のインスタンス名) と ETW (サービス名) で
     *  実際に使用される識別名を返します。\n
     *  com_util_tracer_set_name 未呼び出しの場合は自プロセス名です。
     *
     *  @param[in]      handle    com_util_tracer_create の戻り値。
     *  @param[out]     out       識別名を格納するバッファー。NULL の場合は @ref COM_UTIL_ERR_INVALID_ARGUMENT を返します。
     *  @param[in]      out_size  バッファーのバイト数。0 の場合は @ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                            格納に不足する場合は @ref COM_UTIL_ERR_BUFFER_TOO_SMALL を返します。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、@ref COM_UTIL_ERR_BUFFER_TOO_SMALL 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  stopped / started のどちらの状態でも使用できます。
     *
     *  @see            com_util_tracer_set_name
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_tracer_get_name(com_util_tracer *handle, char *out, size_t out_size);

    /**
     *  @brief          インスタンス識別番号を取得します。
     *
     *  @param[in]      handle    com_util_tracer_create の戻り値。
     *  @return         現在のインスタンス識別番号 (0 以上)。handle が NULL または利用不可の場合 -1。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  stopped / started のどちらの状態でも使用できます。
     *
     *  @see            com_util_tracer_set_name
     */
    COM_UTIL_EXPORT int64_t COM_UTIL_API com_util_tracer_get_identifier(com_util_tracer *handle);

    /**
     *  @brief          トレース ファイル名とファイル識別を設定します。
     *
     *  ファイル トレースのデフォルト パス (実行ファイルのディレクトリ配下の
     *  `log/{ファイル名}.log`) に使用するファイル名を設定します。
     *  ファイル名は `{name}` (identifier が 0 の場合) または `{name}_{identifier}` です。\n
     *  name に NULL を指定した場合はプロセス名 (実行ファイルのベース名。Windows は末尾の
     *  `.exe` を除く) を使用します。明示設定した名前には `.exe` の除去を適用しません。\n
     *  本関数は OS トレースの識別名 (com_util_tracer_set_name) には影響しません。\n
     *  com_util_tracer_set_file_level で出力ファイル パスを明示設定している場合、
     *  本設定はデフォルト パスの解決に使用されないため効果を持ちません。
     *
     *  @param[in]      handle      com_util_tracer_create の戻り値。
     *  @param[in]      name        ファイル名。NULL でプロセス名を使用。
     *  @param[in]      identifier  ファイル識別番号 (0 以上)。0 でサフィックスなし。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、@ref COM_UTIL_ERR_OUT_OF_MEMORY 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  stopped 状態でのみ有効です。started 状態では @ref COM_UTIL_ERR_UNKNOWN を返します。
     *
     *  @see            com_util_tracer_get_file_name
     *  @see            com_util_tracer_get_file_identifier
     *  @see            com_util_tracer_set_file_level
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_tracer_set_file_name(com_util_tracer *handle, const char *name,
                                                                   int64_t identifier);

    /**
     *  @brief          解決済みのトレース ファイル名 (ファイル識別サフィックス込み) を取得します。
     *
     *  ファイル トレースのデフォルト パスで実際に使用されるファイル名
     *  (拡張子 `.log` を除く) を返します。\n
     *  com_util_tracer_set_file_name 未呼び出しの場合はプロセス名
     *  (実行ファイルのベース名。Windows は末尾の `.exe` を除く) です。
     *
     *  @param[in]      handle    com_util_tracer_create の戻り値。
     *  @param[out]     out       ファイル名を格納するバッファー。NULL の場合は @ref COM_UTIL_ERR_INVALID_ARGUMENT を返します。
     *  @param[in]      out_size  バッファーのバイト数。0 の場合は @ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                            格納に不足する場合は @ref COM_UTIL_ERR_BUFFER_TOO_SMALL を返します。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、@ref COM_UTIL_ERR_BUFFER_TOO_SMALL 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  stopped / started のどちらの状態でも使用できます。
     *
     *  @see            com_util_tracer_set_file_name
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_tracer_get_file_name(com_util_tracer *handle, char *out, size_t out_size);

    /**
     *  @brief          ファイル識別番号を取得します。
     *
     *  @param[in]      handle    com_util_tracer_create の戻り値。
     *  @return         現在のファイル識別番号 (0 以上)。handle が NULL または利用不可の場合 -1。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  stopped / started のどちらの状態でも使用できます。
     *
     *  @see            com_util_tracer_set_file_name
     */
    COM_UTIL_EXPORT int64_t COM_UTIL_API com_util_tracer_get_file_identifier(com_util_tracer *handle);

    /**
     *  @brief          OS トレース (EventLog / syslog) の現在のスレッショルド レベルを取得します。
     *
     *  OS トレースは Windows ではイベント ログ (EventLog)、Linux では syslog を指します。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @return         現在のスレッショルド レベル。handle が NULL 時は COM_UTIL_TRACE_LEVEL_NONE。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  返される値は取得時点のスナップショットです。
     */
    COM_UTIL_EXPORT com_util_trace_level COM_UTIL_API com_util_tracer_get_os_level(com_util_tracer *handle);

    /**
     *  @brief          OS トレース (EventLog / syslog) のスレッショルド レベルを設定します。
     *
     *  OS トレースは Windows ではイベント ログ (EventLog)、Linux では syslog を指します。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @param[in]      level    新しいスレッショルド レベル (com_util_trace_level)。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  stopped / started のどちらの状態でも有効です。変更は排他制御下で原子的に反映され、
     *  旧閾値と新閾値の両方で出力対象となるトレースを取りこぼしません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_tracer_set_os_level(com_util_tracer *handle, com_util_trace_level level);

    /**
     *  @brief          ETW トレースの現在のスレッショルド レベルを取得します。
     *
     *  ETW は Windows 専用の独立した診断チャネルです。\n
     *  Linux では ETW が存在しないため、常に COM_UTIL_TRACE_LEVEL_NONE を返します。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @return         現在のスレッショルド レベル。handle が NULL 時または Linux では
     *                  COM_UTIL_TRACE_LEVEL_NONE。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  返される値は取得時点のスナップショットです。
     */
    COM_UTIL_EXPORT com_util_trace_level COM_UTIL_API com_util_tracer_get_etw_level(com_util_tracer *handle);

    /**
     *  @brief          ETW トレースのスレッショルド レベルを設定します。
     *
     *  ETW は Windows 専用の独立した診断チャネルであり、OS トレース (EventLog) とは
     *  別の軸として制御します。デフォルトは COM_UTIL_TRACER_DEFAULT_ETW_LEVEL です。\n
     *  Linux では ETW が存在しないため、本関数は何もせず @ref COM_UTIL_OK を返します。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @param[in]      level    新しいスレッショルド レベル (com_util_trace_level)。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  stopped / started のどちらの状態でも有効です。変更は排他制御下で原子的に反映され、
     *  旧閾値と新閾値の両方で出力対象となるトレースを取りこぼしません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_tracer_set_etw_level(com_util_tracer *handle, com_util_trace_level level);

    /**
     *  @brief          ファイル トレースの現在のスレッショルド レベルを取得します。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @return         現在のスレッショルド レベル。handle が NULL 時は COM_UTIL_TRACE_LEVEL_NONE。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  返される値は取得時点のスナップショットです。
     */
    COM_UTIL_EXPORT com_util_trace_level COM_UTIL_API com_util_tracer_get_file_level(com_util_tracer *handle);

    /**
     *  @brief          ファイル トレースの出力先と設定を変更します。
     *
     *  本関数は設定の記録のみを行い、トレース ファイルは com_util_tracer_start 時に開かれます。\n
     *  そのため、出力ファイル パスの妥当性 (オープン可否) は本関数ではなく
     *  com_util_tracer_start の戻り値で報告されます。
     *
     *  path に NULL を指定した場合はデフォルト パスを使用します。
     *  デフォルト パスは実行ファイルのディレクトリ配下の `log/{ファイル名}.log` であり、
     *  ファイル名は start 時点の設定 (com_util_tracer_set_file_name のファイル名とファイル識別)
     *  で解決されます。ファイル名のデフォルトはプロセス名です
     *  (例: `myapp` または Windows の `myapp.exe` → `log/myapp.log`)。\n
     *  実行ファイル パスの取得に失敗した場合は、カレント ディレクトリからの相対パス
     *  `log/{ファイル名}.log` へ出力します。\n
     *  ファイル トレースを無効化するには level に COM_UTIL_TRACE_LEVEL_NONE を指定します。
     *
     *  flags は com_util_trace_file_sink_create にそのまま渡されます。\n
     *  flags に 0 を指定した場合、ファイル トレースは単一プロセス専用になります。
     *  このモードは OS の排他的オープンを使用しないため、呼び出し側が他プロセスから
     *  同一パスへ書き込まないことを保証してください。\n
     *  @ref COM_UTIL_TRACE_FILE_SINK_SHARED を指定すると、
     *  複数プロセスから同一パスへ書き込むための調停を有効にします。
     *  詳細は com_util_trace_file_sink_create を参照してください。
     *
     *  @param[in]      handle       com_util_tracer_create の戻り値。
     *  @param[in]      path         出力ファイル パス。NULL でデフォルト パスを使用。
     *  @param[in]      level        ファイル トレースのスレッショルド レベル。
     *                               COM_UTIL_TRACE_LEVEL_NONE でファイル トレースを無効化。
     *  @param[in]      max_bytes    1 ファイルあたりの最大バイト数。0 で既定値を使用。
     *  @param[in]      generations  保持する旧世代数。0 以下で既定値を使用。
     *  @param[in]      flags        動作フラグ (@ref COM_UTIL_TRACE_FILE_SINK_SHARED の OR 結合、または 0)。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_OUT_OF_MEMORY 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            started 中の即時反映
     *  started 状態でも呼び出せます。この場合は設定を記録するだけでなく、変更を即座に反映します。\n
     *  level に COM_UTIL_TRACE_LEVEL_NONE を指定するとファイル出力を停止します。\n
     *  出力ファイル パスや max_bytes / generations / flags を変更した場合 (または無効状態から
     *  有効化した場合) は、新しい設定でトレース ファイルを開き直します。新しいファイルのオープンに
     *  失敗した場合は、開いていたファイルと従来の設定を保持したまま @ref COM_UTIL_ERR_UNKNOWN を返します。\n
     *  パスとパラメーターが現状と一致し、しきい値レベルのみを変更する場合はファイルを開き直しません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  stopped / started のどちらの状態でも有効です。変更は排他制御下で原子的に反映され、
     *  旧閾値と新閾値の両方で出力対象となるトレースを取りこぼしません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_tracer_set_file_level(com_util_tracer *handle, const char *path,
                                                                    com_util_trace_level level, size_t max_bytes,
                                                                    int generations, int flags);

    /**
     *  @brief          stderr トレースの現在のスレッショルド レベルを取得します。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @return         現在のスレッショルド レベル。handle が NULL 時は COM_UTIL_TRACE_LEVEL_NONE。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  返される値は取得時点のスナップショットです。
     */
    COM_UTIL_EXPORT com_util_trace_level COM_UTIL_API com_util_tracer_get_stderr_level(com_util_tracer *handle);

    /**
     *  @brief          stderr トレースのスレッショルド レベルを設定します。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @param[in]      level    新しいスレッショルド レベル (com_util_trace_level)。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  stopped / started のどちらの状態でも有効です。変更は排他制御下で原子的に反映され、
     *  旧閾値と新閾値の両方で出力対象となるトレースを取りこぼしません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_tracer_set_stderr_level(com_util_tracer *handle,
                                                                      com_util_trace_level level);

    /**
     *  @brief          トレース プロバイダーを終了し、リソースを解放します。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。NULL は無視。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  解放対象の @p handle を他スレッドが使用していないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_tracer_dispose(com_util_tracer *handle);

    /**
     *  @brief          トレース フックを登録します。
     *
     *  フックはプロセス内コールバックとして動作し、
     *  _com_util_tracer_write 系関数を経由したすべての trace 呼び出しを受信できます。\n
     *  フィルター条件はなく、COM_UTIL_TRACE_LEVEL_NONE で要求された呼び出しも含め
     *  すべての trace イベントが通知されます。\n
     *  タイムスタンプは解決済みの状態でコールバックに渡されます。\n
     *  複数のフックを登録した場合はチェーンとして順次呼び出されます。
     *  コールバック内で com_util_tracer_call_next_hook を呼ぶことでチェーンを継続できます。\n
     *  本関数は stopped 状態でのみ有効です。
     *
     *  @param[in]      handle   com_util_tracer_create の戻り値。
     *  @param[in]      fn       コールバック関数。NULL は無効。
     *  @param[in]      context  コールバックに渡す任意のコンテキスト。NULL 可。
     *  @return         成功時は com_util_tracer_remove_hook() に渡すフック エントリを返します。
     *                  @p handle または @p fn が NULL、started 状態、またはメモリ不足の場合は
     *                  NULL を返します。
     *
     *  @par            使用例
        @code{.c}
        com_util_tracer_hook_entry *entry =
            com_util_tracer_set_hook(tracer, my_hook, NULL);
        com_util_tracer_start(tracer);
        // ... trace 処理 ...
        com_util_tracer_stop(tracer);
        com_util_tracer_remove_hook(tracer, entry);
        @endcode
     *
     *  @par            スレッド セーフ
     *  本関数は stopped 状態でスレッド セーフです。
     */
    COM_UTIL_EXPORT com_util_tracer_hook_entry *COM_UTIL_API com_util_tracer_set_hook(com_util_tracer *handle,
                                                                                      com_util_tracer_hook_fn fn,
                                                                                      void *context);

    /**
     *  @brief          登録済みトレース フックを解除します。
     *
     *  com_util_tracer_set_hook で登録したフックを解除し、エントリのメモリを解放します。\n
     *  本関数は stopped 状態でのみ有効です。
     *
     *  @param[in]      handle      com_util_tracer_create の戻り値。
     *  @param[in]      hook_entry  com_util_tracer_set_hook の戻り値。NULL は無視。
     *
     *  @par            スレッド セーフ
     *  本関数は stopped 状態でスレッド セーフです。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_tracer_remove_hook(com_util_tracer *handle,
                                                                  com_util_tracer_hook_entry *hook_entry);

    /**
     *  @brief          フック チェーンを継続します。
     *
     *  コールバック内から呼び出し、前のフックへ処理を継続させます。\n
     *  @p prev が NULL の場合は何もしません (チェーン末端)。
     *
     *  @param[in]      prev       コールバックに渡された `prev` 引数。
     *  @param[in]      handle     trace を行った tracer ハンドル。
     *  @param[in]      level      trace レベル。
     *  @param[in]      timestamp  解決済みタイムスタンプ。
     *  @param[in]      message    解決済みメッセージ文字列。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  フック コールバック内から複数スレッドで同時に呼び出せます。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_tracer_call_next_hook(com_util_tracer_hook_entry *prev,
                                                                     com_util_tracer *handle,
                                                                     com_util_trace_level level,
                                                                     const com_util_timespec *timestamp,
                                                                     const char *message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* ===== ソース位置自動付与マクロ ===== */

/**
 *  @brief  HEX 出力用ラベル セパレータを返す内部ヘルパー関数です。
 *  @internal
 */
static inline const char *_com_util_tracer_hex_sep(const char *message)
{
    if (message != NULL && message[0] != '\0')
    {
        return " ";
    }
    return "";
}

/**
 *  @brief  HEX 出力用ラベル文字列を返す内部ヘルパー関数 (NULL ガード) です。
 *  @internal
 */
static inline const char *_com_util_tracer_hex_msg(const char *message)
{
    if (message != NULL)
    {
        return message;
    }
    return "";
}

/**
 *  @brief          ソース位置付きメッセージを組み立てて tracer へ書き込む内部ヘルパーです。
 *  @param[in]      handle     com_util_tracer_create の戻り値。
 *  @param[in]      level      トレース レベル (com_util_trace_level)。
 *  @param[in]      timestamp  使用する実時刻。NULL の場合は API 内部で現在時刻を取得。
 *  @param[in]      file       出力に付与するソース ファイル名。
 *  @param[in]      line       出力に付与するソース行番号。
 *  @param[in]      message    null 終端 UTF-8 文字列。NULL の場合はソース位置のみを出力。
 *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
 *  @internal
 *
 *  @par            スレッド セーフ
 *  本関数はスレッド セーフです。\n
 *  内部で _com_util_tracer_writef に委譲しており、複数スレッドから同時に呼び出せます。
 */
static inline int _com_util_tracer_write_with_source(com_util_tracer *handle, com_util_trace_level level,
                                                     const com_util_timespec *timestamp, const char *file, int line,
                                                     const char *message)
{
    if (message != NULL)
    {
        return _com_util_tracer_writef(handle, level, timestamp, "[%s:%d] %s", file, line, message);
    }

    return _com_util_tracer_writef(handle, level, timestamp, "[%s:%d]", file, line);
}

/**
 *  @brief          ソース ファイル名と行番号を自動付与する com_util_tracer_write マクロです。
 */
#define com_util_tracer_write(handle, level, timestamp, message) \
    _com_util_tracer_write_with_source((handle), (level), (timestamp), com_util_path_basename(__FILE__), __LINE__, \
                                       (message))

/**
 *  @brief          ソース ファイル名と行番号を自動付与する com_util_tracer_writef マクロです。
 */
#define com_util_tracer_writef(handle, level, timestamp, fmt, ...) \
    _com_util_tracer_writef((handle), (level), (timestamp), "[%s:%d] " fmt, com_util_path_basename(__FILE__), \
                            __LINE__, ##__VA_ARGS__)

/**
 *  @brief          ソース ファイル名と行番号を自動付与する com_util_tracer_write_hex マクロです。
 */
#define com_util_tracer_write_hex(handle, level, timestamp, data, size, message) \
    _com_util_tracer_write_hexf((handle), (level), (timestamp), (data), (size), "[%s:%d]%s%s", \
                                com_util_path_basename(__FILE__), __LINE__, _com_util_tracer_hex_sep(message), \
                                _com_util_tracer_hex_msg(message))

/**
 *  @brief          ソース ファイル名と行番号を自動付与する com_util_tracer_write_hexf マクロです。
 */
#define com_util_tracer_write_hexf(handle, level, timestamp, data, size, fmt, ...) \
    _com_util_tracer_write_hexf((handle), (level), (timestamp), (data), (size), "[%s:%d] " fmt, \
                                com_util_path_basename(__FILE__), __LINE__, ##__VA_ARGS__)

/** @} */

#endif /* COM_UTIL_TRACER_H */
