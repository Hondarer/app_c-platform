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
        unsigned int pad; /**< 64bit 環境で native_handle のアラインメントを明示する予約領域。 */
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
     *  @par            スレッド セーフ
     *  本関数は内部に共有状態を持ちません。ただし、Windows で UAC を表示するため、
     *  通常はメイン スレッドまたはユーザー操作に応答するスレッドから呼び出してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_process_run_elevated_if_needed(const char *arguments, int *exit_code,
                                                                             int *handled);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_PROCESS_H */
