#ifndef COM_UTIL_EVENTLOG_H
#define COM_UTIL_EVENTLOG_H

#include <com_util/base/platform.h>
#include <com_util/com_util_export.h>
#include <inttypes.h>

/**
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @{
 */

/**
 *  @file           eventlog.h
 *  @brief          Windows イベント ログ (EventLog) ヘルパーライブラリです。
 *
 *  Windows のアプリケーション イベント ログへ書き込むための
 *  ヘルパー関数群と、共通イベント ソースの登録/削除 API を提供します。\n
 *  Windows 専用ライブラリです。呼び出し元は @c \#if defined(PLATFORM_WINDOWS) の
 *  中でのみ使用してください。
 *
 *  イベント ソースは com_util 全体で共通とし、ソース名には
 *  @c COM_UTIL_TRACER_DEFAULT_PROVIDER_NAME を用います。\n
 *  分析性を高めるため、トレース レベル毎にイベント タイプとイベント ID を
 *  分けて書き込みます。
 *
 *  @hideincludedbygraph
 *
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

/** @name イベント ソース登録ステータス コード */
/** @{ */

/** 成功。 */
#define COM_UTIL_EVENTLOG_OK 0
/** パラメーター エラー (NULL など)。 */
#define COM_UTIL_EVENTLOG_ERR_PARAM (-1)
/** 権限不足 (HKLM への書き込みには管理者権限が必要)。 */
#define COM_UTIL_EVENTLOG_ERR_ACCESS (-2)
/** その他のシステム エラー。 */
#define COM_UTIL_EVENTLOG_ERR_SYSTEM (-3)

/** @} */

/**
 *  @brief          EventLog が扱うトレース レベルの数です。
 *
 *  レベル毎にイベント ID とカテゴリを割り当てるため、登録時の
 *  CategoryCount に使用します。
 */
#define COM_UTIL_EVENTLOG_LEVEL_COUNT 6

#if defined(PLATFORM_WINDOWS)

/* ===== 不透明ハンドル型 ===== */

/** EventLog シンク ハンドル (不透明型)。 */
typedef struct com_util_eventlog_sink com_util_eventlog_sink;

    /* ===== API 関数 ===== */

    #ifdef __cplusplus
extern "C"
{
    #endif /* __cplusplus */

    /**
     *  @brief          イベント ソースに対する書き込みハンドルを取得します。
     *
     *  @param[in]      source_name  イベント ソース名 (UTF-8)。NULL は失敗。
     *  @return         成功時はハンドル、失敗時は NULL を返します。
     *
     *  内部で @c RegisterEventSourceW を呼び出します。\n
     *  ソースが未登録でも呼び出しは成功しますが、その場合 Event Viewer 上での
     *  表示はソース名の解決が行われません。事前に
     *  com_util_eventlog_register_source() で登録してください。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT com_util_eventlog_sink *COM_UTIL_API com_util_eventlog_sink_create(const char *source_name);

    /**
     *  @brief          イベント ログへ UTF-8 メッセージを書き込みます。
     *
     *  @param[in]      handle         com_util_eventlog_sink_create の戻り値。NULL は無視。
     *  @param[in]      level          トレース レベル (0=CRITICAL / 1=ERROR / 2=WARNING /
     *                                 3=INFO / 4=VERBOSE / 5=DEBUG)。
     *  @param[in]      file_identifier      ファイル識別番号。0 の場合は Event Viewer 表示では省略。
     *  @param[in]      instance_name        インスタンス名 (UTF-8)。NULL の場合は空文字列。
     *  @param[in]      instance_identifier  インスタンス識別番号。0 の場合は Event Viewer 表示では省略。
     *  @param[in]      message              null 終端 UTF-8 文字列。NULL は無視。
     *  @return         成功時は 0、失敗時は -1 を返します。
     *
     *  level をイベント タイプ (Error / Warning / Information) とイベント ID に
     *  写像して @c ReportEventW を呼び出します。\n
     *  EventLog の置換文字列は、メッセージ文字列、実行体ファイルパス、ファイル識別子、
     *  インスタンス名、インスタンス識別子の 5 件です。\n
     *  Event Viewer の「全般」では、実行体ファイルパス、インスタンス名、メッセージ文字列の
     *  3 行を表示します。識別子が 0 以外の場合は各行に @c _識別子 を付与します。\n
     *  実行ファイル絶対パスはプロセス内で初回だけ解決され、以後はキャッシュを使用します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  ReportEventW は複数スレッドからの同時呼び出しをサポートしています。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_eventlog_sink_write(com_util_eventlog_sink *handle, int level,
                                                                  int64_t file_identifier, const char *instance_name,
                                                                  int64_t instance_identifier, const char *message);

    /**
     *  @brief          イベント ログ書き込みハンドルを解放します。
     *
     *  @param[in]      handle   com_util_eventlog_sink_create の戻り値。NULL は無視。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  解放対象の @p handle を他スレッドが使用していないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_eventlog_sink_dispose(com_util_eventlog_sink *handle);

    /**
     *  @brief          共通イベント ソースをレジストリに登録します。
     *
     *  @param[in]      source_name        イベント ソース名 (UTF-8)。NULL は失敗。
     *  @param[in]      message_file_path  メッセージ リソース (MESSAGETABLE) を持つファイルの
     *                                     絶対パス (UTF-8) です。NULL の場合は登録しません。
     *  @return         登録に成功した場合は COM_UTIL_EVENTLOG_OK を返します。引数不正の場合は
     *                  COM_UTIL_EVENTLOG_ERR_PARAM、権限不足の場合は COM_UTIL_EVENTLOG_ERR_ACCESS、
     *                  その他のシステム エラーの場合は COM_UTIL_EVENTLOG_ERR_SYSTEM を返します。
     *
     *  @c HKLM\\SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\{source_name}
     *  キーを作成し、TypesSupported と CategoryCount を設定します。\n
     *  @p message_file_path が非 NULL の場合は EventMessageFile と CategoryMessageFile に
     *  そのパスを設定します。これにより Event Viewer はメッセージ本文とカテゴリ名を解決し、
     *  イベント ID の説明が見つからない旨の補完文を表示しなくなります。\n
     *  @p message_file_path が NULL の場合はメッセージ ファイルを設定しません (補完文が付きます)。\n
     *  HKLM への書き込みには管理者権限が必要です。権限不足の場合は
     *  COM_UTIL_EVENTLOG_ERR_ACCESS を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_eventlog_register_source(const char *source_name,
                                                                       const char *message_file_path);

    /**
     *  @brief          共通イベント ソースの登録を削除します。
     *
     *  @param[in]      source_name  イベント ソース名 (UTF-8)。NULL は失敗。
     *  @return         削除に成功した場合、または登録が存在しない場合は COM_UTIL_EVENTLOG_OK を返します。
     *                  引数不正の場合は COM_UTIL_EVENTLOG_ERR_PARAM、権限不足の場合は
     *                  COM_UTIL_EVENTLOG_ERR_ACCESS、その他のシステム エラーの場合は
     *                  COM_UTIL_EVENTLOG_ERR_SYSTEM を返します。
     *
     *  対応するレジストリ キーを削除します。\n
     *  既に存在しない場合も COM_UTIL_EVENTLOG_OK を返します (冪等)。\n
     *  HKLM への書き込みには管理者権限が必要です。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_eventlog_unregister_source(const char *source_name);

    #ifdef __cplusplus
}
    #endif /* __cplusplus */

#endif /* PLATFORM_WINDOWS */

/** @} */

#endif /* COM_UTIL_EVENTLOG_H */
