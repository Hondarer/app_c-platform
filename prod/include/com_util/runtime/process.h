/**
 *******************************************************************************
 *  @file           process.h
 *  @brief          プロセス情報取得 API。
 *  @author         Tetsuo Honda
 *  @date           2026/06/07
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_PROCESS_H
#define COM_UTIL_PROCESS_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
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

    /**
     *  @brief          現在のプロセスの実行ファイル本体の絶対パスを取得します。
     *  @param[out]     out_path      絶対パス (UTF-8) の格納先。NULL を渡してはなりません。
     *  @param[in]      out_path_sz   @p out_path のサイズ (バイト)。0 を渡してはなりません。
     *  @return         成功時は 0、失敗時は -1 を返します。
     *
     *  com_util_module_get_path() が関数アドレスの所属モジュールを返す (Windows では
     *  DLL を指しうる) のに対し、本関数は常にプロセス本体の実行ファイルのパスを返します。\n
     *  返されるパスは UTF-8 文字列で、パス セパレーターは '/' で統一されます。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_process_get_executable_path(char *out_path, size_t out_path_sz);

#define COM_UTIL_PROCESS_WAIT_FOREVER INT_MAX /**< タイムアウトなしで待機する (INT_MAX)。 */
#define COM_UTIL_PROCESS_NO_WAIT      0       /**< 即時リターン (タイムアウト 0 ms)。 */

    /** @brief プロセス操作の結果コード。 */
    typedef enum
    {
        COM_UTIL_PROCESS_OK = 0,               /**< 成功。 */
        COM_UTIL_PROCESS_TIMEOUT = 1,          /**< タイムアウト。 */
        COM_UTIL_PROCESS_INVALID_ARGUMENT = 2, /**< 引数が不正。 */
        COM_UTIL_PROCESS_SYSTEM_ERROR = 3,     /**< OS/システム エラー。 */
        COM_UTIL_PROCESS_UNSUPPORTED = 4       /**< 操作がサポートされない。 */
    } com_util_process_result_t;

    /** @brief 子プロセスの標準入出力ハンドルの扱い。 */
    typedef enum
    {
        COM_UTIL_PROCESS_STDIO_INHERIT = 0,      /**< 親プロセスの標準ハンドルを継承します。 */
        COM_UTIL_PROCESS_STDIO_NULL_DEVICE = 1,  /**< null device へ接続します。 */
        COM_UTIL_PROCESS_STDIO_NATIVE_HANDLE = 2 /**< native_handle で指定した OS ハンドルを使います。 */
    } com_util_process_stdio_mode_t;

    /**
     *  @brief          子プロセスの標準入出力指定。
     *
     *  Linux では @p native_handle をファイル ディスクリプタ、Windows では HANDLE として扱います。\n
     *  呼び出し側が渡した native handle の所有権は移動しません。
     */
    typedef struct com_util_process_stdio
    {
        com_util_process_stdio_mode_t mode; /**< 標準入出力の扱い。 */
#if defined(ARCH_X64)
        unsigned int pad; /**< x64 で native_handle のアラインメントを明示する予約領域。 */
#endif
        intptr_t native_handle; /**< OS ネイティブ ハンドル。 */
    } com_util_process_stdio_t;

    /** @brief 子プロセス起動オプション。 */
    typedef struct com_util_process_options
    {
        char *const *argv;                    /**< コマンドと引数の配列 (NULL 終端)。NULL を渡してはなりません。 */
        char *const *env_overrides;           /**< 追加・上書きする KEY=VALUE 配列 (NULL 終端)。NULL 可。 */
        const char *working_directory;        /**< 作業ディレクトリ。NULL の場合は親の作業ディレクトリを継承します。 */
        com_util_process_stdio_t stdin_spec;  /**< stdin 指定。 */
        com_util_process_stdio_t stdout_spec; /**< stdout 指定。 */
        com_util_process_stdio_t stderr_spec; /**< stderr 指定。 */
    } com_util_process_options_t;

    typedef struct com_util_process com_util_process; /**< 子プロセス ハンドル。 */

    /**
     *  @brief          子プロセスを起動します。
     *  @param[in]      options  起動オプション。NULL を渡してはなりません。
     *  @param[out]     process  起動したプロセス ハンドルの格納先。NULL を渡してはなりません。
     *  @return         結果コードを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  呼び出しごとに独立したプロセス ハンドルを生成します。
     */
    COM_UTIL_EXPORT com_util_process_result_t COM_UTIL_API
    com_util_process_start(const com_util_process_options_t *options, com_util_process **process);

    /**
     *  @brief          子プロセスの終了を待機します。
     *  @param[in]      process     対象のプロセス ハンドル。NULL を渡してはなりません。
     *  @param[in]      timeout_ms  タイムアウト (ms)。@ref COM_UTIL_PROCESS_WAIT_FOREVER または
     *                              @ref COM_UTIL_PROCESS_NO_WAIT も指定可能です。
     *  @return         結果コードを返します。
     */
    COM_UTIL_EXPORT com_util_process_result_t COM_UTIL_API com_util_process_wait(com_util_process *process,
                                                                                 int timeout_ms);

    /**
     *  @brief          子プロセスの終了コードを取得します。
     *  @param[in]      process    対象のプロセス ハンドル。NULL を渡してはなりません。
     *  @param[out]     exit_code  終了コードの格納先。NULL を渡してはなりません。
     *  @return         結果コードを返します。
     */
    COM_UTIL_EXPORT com_util_process_result_t COM_UTIL_API com_util_process_get_exit_code(com_util_process *process,
                                                                                          int *exit_code);

    /**
     *  @brief          子プロセスを強制終了します。
     *  @param[in]      process  対象のプロセス ハンドル。NULL を渡してはなりません。
     *  @return         結果コードを返します。
     */
    COM_UTIL_EXPORT com_util_process_result_t COM_UTIL_API com_util_process_terminate(com_util_process *process);

    /**
     *  @brief          子プロセス ハンドルを破棄します。
     *  @param[in]      process  破棄するプロセス ハンドル。NULL 可。
     *
     *  実行中のプロセスは終了しません。\n
     *  実行中プロセスを終了する場合は、先に com_util_process_terminate() を呼び出してください。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_process_destroy(com_util_process *process);

    /**
     *  @brief          子プロセスを起動し、終了まで同期的に待機します。
     *  @param[in]      options     起動オプション。NULL を渡してはなりません。
     *  @param[in]      timeout_ms  タイムアウト (ms)。
     *  @param[out]     exit_code   終了コードの格納先。NULL を渡してはなりません。
     *  @return         結果コードを返します。
     */
    COM_UTIL_EXPORT com_util_process_result_t COM_UTIL_API
    com_util_process_run_sync(const com_util_process_options_t *options, int timeout_ms, int *exit_code);

    /**
     *  @brief          管理者/root 権限が必要な処理のため、必要に応じて昇格実行します。
     *  @param[in]      arguments  昇格実行時に現在の実行ファイルへ渡す引数文字列。NULL 可。
     *  @param[out]     exit_code  昇格プロセスの終了コード、または 0 の格納先。
     *  @param[out]     handled    昇格プロセスで処理した場合は 0 以外、現プロセスで継続する場合は 0 の格納先。
     *  @return         権限確認または昇格プロセス実行が成功した場合は 0、失敗した場合は -1 を返します。
     *
     *  Windows では、未昇格の場合に UAC を要求して現在の実行ファイルを @p arguments 付きで
     *  再起動し、子プロセスの終了まで待機します。すでに昇格済みの場合は何もしません。\n
     *  Linux では、実効ユーザー ID が root であることを確認します。root でない場合は失敗します。\n
     *  本関数は権限保証のための API であり、Linux で `sudo` などの外部昇格コマンドは実行しません。
     *
     *  @note           Windows で呼び出し元にコンソールがある場合、昇格プロセスへ親コンソールを
     *                  引き継ぐため、昇格プロセスは別ウインドウを表示せず親コンソールへ出力します。\n
     *                  この場合 @p arguments の末尾に内部フラグを付与して再起動するため、昇格
     *                  プロセスは起動直後に com_util_console_attach_parent() を呼び出す必要が
     *                  あります。呼び出さない場合、昇格プロセスの出力は表示されません。\n
     *                  呼び出し元にコンソールが無い場合は、昇格プロセスを通常表示で起動します。
     *
     *  @par            スレッド セーフ
     *  本関数は内部に共有状態を持ちません。ただし、Windows で UAC を表示するため、
     *  通常はメイン スレッドまたはユーザー操作に応答するスレッドから呼び出してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_process_run_elevated_if_needed(const char *arguments, int *exit_code,
                                                                             int *handled);

    /**
     *  @brief          管理者/root 権限が必要な処理のため、必要に応じて昇格実行し、
     *                  昇格プロセスが報告した結果メッセージを取得します。
     *  @param[in]      arguments            昇格実行時に現在の実行ファイルへ渡す引数文字列。NULL 可。
     *  @param[out]     exit_code            昇格プロセスの終了コード、または 0 の格納先。
     *  @param[out]     handled              昇格プロセスで処理した場合は 0 以外、現プロセスで継続する場合は 0 の格納先。
     *  @param[out]     result_message       昇格プロセスが報告したメッセージ (UTF-8) の格納先。NULL 可。
     *  @param[in]      result_message_size  @p result_message のバイト数。
     *  @return         権限確認または昇格プロセス実行が成功した場合は 0、失敗した場合は -1 を返します。
     *
     *  com_util_process_run_elevated_if_needed() はコンソールの再接続を昇格プロセス側に
     *  要求しますが、UAC 昇格直後の親コンソール再割り当ては実機調査でも原因を特定できない
     *  間欠的な書き込み不能 (`ERROR_INVALID_HANDLE`) を起こすことが分かっています。\n
     *  本関数は昇格プロセスのコンソールを一切引き継がず、結果メッセージを一時ファイル経由で
     *  受け渡します。昇格プロセス側は com_util_process_extract_result_target() を起動直後に
     *  呼び出し、処理結果を com_util_process_report_elevated_result() で報告してください。\n
     *  Linux では com_util_process_run_elevated_if_needed() と同じ判定を行い、
     *  @p result_message は変更しません (別プロセスを起動しないため報告の余地がない)。
     *
     *  @par            スレッド セーフ
     *  本関数は内部に共有状態を持ちません。ただし、Windows で UAC を表示するため、
     *  通常はメイン スレッドまたはユーザー操作に応答するスレッドから呼び出してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_process_run_elevated_with_result(const char *arguments, int *exit_code,
                                                                               int *handled, char *result_message,
                                                                               size_t result_message_size);

    /**
     *  @brief          argv から結果報告先フラグを取り出します。
     *  @param[in,out]  argc  引数の数へのポインター。NULL 可。
     *  @param[in,out]  argv  引数配列。NULL 可。
     *  @return         報告先フラグを検出した場合は 1、そうでない場合は 0 を返します。
     *
     *  com_util_process_run_elevated_with_result() が付与したフラグを検出し、後続の
     *  com_util_process_report_elevated_result() のために報告先を保持します。検出した
     *  フラグは @p argv から取り除き、@p argc を 1 減らします。\n
     *  プログラム開始直後、引数解析より前に 1 度だけ呼び出してください。\n
     *  Linux では何もせず 0 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  プロセス起動直後のシングル スレッド フェーズで呼び出してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_process_extract_result_target(int *argc, char **argv);

    /**
     *  @brief          昇格プロセスから、呼び出し元プロセスへ結果メッセージを報告します。
     *  @param[in]      message  報告するメッセージ (UTF-8)。NULL を渡してはなりません。
     *  @return         成功時は 0、報告先が無い場合や失敗時は -1 を返します。
     *
     *  com_util_process_extract_result_target() で報告先を検出している場合のみ、
     *  そのファイルへ @p message を書き込みます。報告先を検出していない場合
     *  (com_util_process_run_elevated_with_result() 経由で起動されていない場合) は
     *  何も行わず -1 を返すため、呼び出し元はその場合に自分自身の標準出力へ
     *  直接表示してください。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  内部状態は com_util_process_extract_result_target() が設定したものを参照します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_process_report_elevated_result(const char *message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_PROCESS_H */
