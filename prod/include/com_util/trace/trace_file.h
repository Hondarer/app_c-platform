#ifndef COM_UTIL_TRACE_FILE_H
#define COM_UTIL_TRACE_FILE_H

/**
 *  @file           trace_file.h
 *  @brief          ファイル ベース トレース プロバイダー ライブラリ。
 *
 *  トレース メッセージをローテーション付きテキスト ファイルへ同期書き込みする
 *  クロスプラットフォーム プロバイダーです。\n
 *  com_util_etw_provider (Windows ETW) および com_util_syslog_sink (Linux syslog) と
 *  同じ init / write / dispose インターフェースを提供します。
 *
 *  @par            出力フォーマット
 *  @code
   2026-03-31 12:34:56.789 I メッセージテキスト
 *  @endcode
 *  レベル文字: C=CRITICAL / E=ERROR / W=WARNING / I=INFO / V=VERBOSE
 *
 *  @hideincludedbygraph
 *
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#include <stddef.h>
#include <com_util/base/platform.h>
#include <com_util/clock/clock.h>
#include <com_util/com_util_export.h>
#include <com_util/trace/tracer.h>

/**
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @{
 */

/* ===== 設定定数 ===== */

/**
 *  @brief          トレース ファイル 1 世代あたりの既定最大サイズ (バイト)。
 *
 *  この値を超えるとローテーションが実行されます。\n
 *  com_util_trace_file_sink_create の max_bytes に 0 を指定した場合に使用されます。
 */
#define COM_UTIL_TRACE_FILE_SINK_DEFAULT_MAX_BYTES ((size_t)(10 * 1024 * 1024))

/**
 *  @brief          保持するトレース ファイル世代数の既定値。
 *
 *  ローテーション時に path.1 〜 path.N のファイルを保持します。\n
 *  com_util_trace_file_sink_create の generations に 0 以下を指定した場合に使用されます。
 */
#define COM_UTIL_TRACE_FILE_SINK_DEFAULT_GENERATIONS 5

/**
 *  @brief          複数プロセスからの同時書き込みを有効にするフラグ。
 *
 *  com_util_trace_file_sink_create の flags に指定します。\n
 *  指定すると、出力ファイルを共有書き込み可能なモードで開き、
 *  ローテーションをロック ファイル `<path>.lock` によるプロセス間排他のもとで実行します。\n
 *  指定しない場合は単一プロセス専用となり、ロック ファイルは作成されません。
 */
#define COM_UTIL_TRACE_FILE_SINK_SHARED (1 << 0)

/* ===== 不透明ハンドル型 ===== */

/** ファイル トレース プロバイダー ハンドル (不透明型)。 */
typedef struct com_util_trace_file_sink com_util_trace_file_sink;

/* ===== API 関数 ===== */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          ファイル トレース プロバイダーを初期化する。
     *
     *  指定されたファイル パスへの書き込みを開始します。\n
     *  ファイルが存在する場合は追記します。存在しない場合は新規作成します。\n
     *  親ディレクトリが存在しない場合は再帰的に自動生成します (`mkdir -p` 相当)。\n
     *  max_bytes に 0 を指定した場合は COM_UTIL_TRACE_FILE_SINK_DEFAULT_MAX_BYTES を使用します。\n
     *  generations に 0 以下を指定した場合は COM_UTIL_TRACE_FILE_SINK_DEFAULT_GENERATIONS を使用します。
     *
     *  flags に 0 を指定した場合は単一プロセス専用です。出力ファイルを共有書き込み禁止で開くため、
     *  Windows では他プロセスが同じファイルを書き込み用に開けません。\n
     *  flags に @ref COM_UTIL_TRACE_FILE_SINK_SHARED を指定した場合は、
     *  複数プロセスから同一パスへ書き込めます。このとき:
     *  - 各書き込みは OS のアトミック追記で行い、サイズ判定はファイルの実サイズで行います。
     *  - ローテーションはロック ファイル `<path>.lock` によるプロセス間排他のもとで実行します。
     *    ロック ファイルは常設で、削除されません。
     *  - ロック ファイルのオープンに失敗した場合は NULL を返します。
     *
     *  @param[in]      path         出力ファイル パス。NULL の場合は NULL を返します。
     *  @param[in]      max_bytes    1 ファイルあたりの最大バイト数。0 でデフォルト値を使用。
     *  @param[in]      generations  保持する旧世代数。0 以下でデフォルト値を使用。
     *  @param[in]      flags        動作フラグ (@ref COM_UTIL_TRACE_FILE_SINK_SHARED の OR 結合、または 0)。
     *                               負値を渡した場合は NULL を返します。
     *  @return         成功時: ハンドル。失敗時: NULL。
     *
     *  @warning        追記のアトミック性が保証されないファイルシステム (NFS など) では、
     *                  共有モードでも行が混入する場合があります。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。各呼び出しは独立したハンドルを生成します。
     */
    COM_UTIL_EXPORT com_util_trace_file_sink *COM_UTIL_API com_util_trace_file_sink_create(const char *path,
                                                                                           size_t max_bytes,
                                                                                           int generations, int flags);

    /**
     *  @brief          ファイルへトレース メッセージを書き込む。
     *
     *  @param[in]      handle     com_util_trace_file_sink_create の戻り値。NULL は無視。
     *  @param[in]      level      トレース レベル。
     *  @param[in]      timestamp  使用する実時刻。NULL の場合は API 内部で現在時刻を取得。
     *                             不正な明示タイムスタンプが渡された場合も現在時刻で出力を継続し、
     *                             戻り値は -1 を返します。
     *  @param[in]      message    null 終端 UTF-8 文字列。NULL は無視。
     *  @return         成功 0 / 失敗 -1。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部の mutex で保護されており、同一 @p handle に対して複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_trace_file_sink_write(com_util_trace_file_sink *handle, int level,
                                                                    const com_util_realtime_timestamp *timestamp,
                                                                    const char *message);

    /**
     *  @brief          ファイル トレース プロバイダーを終了する。
     *
     *  @param[in]      handle   com_util_trace_file_sink_create の戻り値。NULL は無視。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  解放対象の @p handle を他スレッドが使用していないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_trace_file_sink_dispose(com_util_trace_file_sink *handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_TRACE_FILE_H */
