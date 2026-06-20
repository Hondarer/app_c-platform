/**
 *******************************************************************************
 *  @file           eventlog-register.c
 *  @brief          com_util 共通イベント ソースの登録/削除コマンド。
 *  @author         Tetsuo Honda
 *  @date           2026/06/14
 *  @version        1.0.0
 *
 *  Windows のアプリケーション イベント ログに com_util 共通イベント ソースを
 *  登録/削除します。HKLM への書き込みには管理者権限が必要なため、未昇格時は
 *  UAC 昇格を行います。\n
 *  登録先は @c HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\com_util.tracer
 *  キーです。ソース名 @c com_util.tracer は @c COM_UTIL_TRACER_DEFAULT_PROVIDER_NAME の値で、
 *  登録状態の確認はこのキーの有無で判断できます。\n
 *  Linux ではイベント ログを使用しないため、案内を表示して終了します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include "eventlog-register.h"

#include <com_util/base/platform.h>
#include <com_util/console/console.h>
#include <com_util/runtime/elevated_process.h>
#include <com_util/runtime/process.h>
#include <com_util/trace/tracer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Doxygen コメントは、ヘッダーに記載 */

void eventlog_register_print_usage(const char *argv0)
{
    const char *name = argv0;

    if (name == NULL)
    {
        name = "eventlog-register";
    }

    fprintf(stderr, "使用方法: %s {install|uninstall}\n", name);
    fprintf(stderr, "  install    com_util 共通イベント ソースを登録します (要管理者権限)。\n");
    fprintf(stderr, "  uninstall  com_util 共通イベント ソースの登録を削除します (要管理者権限)。\n");
}

#if defined(PLATFORM_WINDOWS)

    #include <com_util/crt/path.h>
    #include <com_util/trace/eventlog.h>

/**
 *  @brief          本プロセスが昇格ワーカー (UAC 昇格で再起動された側) かどうか。
 *
 *  main() で com_util_elevated_process_extract_result_target() の戻り値を設定する。\n
 *  0 以外の場合、report_status() は標準出力/エラーへ直接出力せず、
 *  com_util_elevated_process_report_result() で呼び出し元プロセスへ報告する。
 */
static int s_is_elevated_worker = 0;

/**
 *  @brief          管理者権限を保証する。必要なら UAC 昇格して再実行する。
 *  @param[in]      command  昇格再実行するサブコマンド ("install" / "uninstall")。
 *  @param[out]     handled  昇格プロセスで処理済みの場合は 0 以外を格納する。
 *  @return         継続可能な場合は 0、失敗時または昇格プロセスの終了コードを返す。
 *
 *  昇格プロセスのコンソールは一切引き継がない。昇格プロセス側が報告した結果メッセージは
 *  本関数が受け取り、終了コードに応じて自分自身の標準出力/エラーへそのまま表示する。
 */
static int ensure_elevated(const char *command, int *handled)
{
    char result_message[256];
    int exit_code;
    int rc;

    if (handled == NULL)
    {
        return EXIT_FAILURE;
    }

    exit_code = EXIT_FAILURE;
    rc =
        com_util_elevated_process_run_with_result(command, &exit_code, handled, result_message, sizeof(result_message));
    if (rc != 0)
    {
        fprintf(stderr, "管理者権限への昇格に失敗しました。\n");
        return EXIT_FAILURE;
    }

    if (*handled != 0)
    {
        if (result_message[0] != '\0')
        {
            if (exit_code == EXIT_SUCCESS)
            {
                printf("%s", result_message);
            }
            else
            {
                fprintf(stderr, "%s", result_message);
            }
        }
        return exit_code;
    }
    return 0;
}

/**
 *  @brief          イベント ソース API のステータスを表示し、終了コードに変換する。
 *  @param[in]      status  com_util_eventlog_register_source / unregister_source の戻り値。
 *  @param[in]      action  操作名 ("登録" / "削除")。
 *  @return         EXIT_SUCCESS / EXIT_FAILURE。
 *
 *  本プロセスが昇格ワーカーの場合は標準出力/エラーへ出力せず、呼び出し元プロセスへ
 *  com_util_elevated_process_report_result() で報告する (ensure_elevated() がそちらで表示する)。
 */
static int report_status(const int status, const char *action)
{
    char message[256];

    if (status == COM_UTIL_EVENTLOG_OK)
    {
        (void)snprintf(message, sizeof(message), "イベント ソース '%s' を%sしました。\n",
                       COM_UTIL_TRACER_DEFAULT_PROVIDER_NAME, action);
        if (s_is_elevated_worker != 0)
        {
            (void)com_util_elevated_process_report_result(message);
        }
        else
        {
            printf("%s", message);
        }
        return EXIT_SUCCESS;
    }
    if (status == COM_UTIL_EVENTLOG_ERR_ACCESS)
    {
        (void)snprintf(message, sizeof(message), "アクセスが拒否されました。管理者として実行してください。\n");
    }
    else if (status == COM_UTIL_EVENTLOG_ERR_PARAM)
    {
        (void)snprintf(message, sizeof(message), "パラメーターが不正です。\n");
    }
    else
    {
        (void)snprintf(message, sizeof(message), "システム エラーにより%sに失敗しました。\n", action);
    }

    if (s_is_elevated_worker != 0)
    {
        (void)com_util_elevated_process_report_result(message);
    }
    else
    {
        fprintf(stderr, "%s", message);
    }
    return EXIT_FAILURE;
}

/**
 *  @brief          共通イベント ソースを登録する。
 *  @return         EXIT_SUCCESS / EXIT_FAILURE。
 */
static int do_install(void)
{
    int handled = 0;
    int rc;
    int status;
    char exe_path[PLATFORM_PATH_MAX];
    const char *message_file;

    rc = ensure_elevated("install", &handled);
    if (rc != 0 || handled != 0)
    {
        return rc;
    }

    /* メッセージ リソースは eventlog-register.exe 自身に埋め込んでいる。
       自身の絶対パスを EventMessageFile / CategoryMessageFile に登録する。 */
    message_file = NULL;
    if (com_util_process_get_executable_path(exe_path, sizeof(exe_path)) == 0)
    {
        message_file = exe_path;
    }
    else
    {
        fprintf(stderr, "実行ファイルのパスを取得できませんでした。メッセージ リソースなしで登録します。\n");
    }

    status = com_util_eventlog_register_source(COM_UTIL_TRACER_DEFAULT_PROVIDER_NAME, message_file);
    return report_status(status, "登録");
}

/**
 *  @brief          共通イベント ソースの登録を削除する。
 *  @return         EXIT_SUCCESS / EXIT_FAILURE。
 */
static int do_uninstall(void)
{
    int handled = 0;
    int rc;
    int status;

    rc = ensure_elevated("uninstall", &handled);
    if (rc != 0 || handled != 0)
    {
        return rc;
    }

    status = com_util_eventlog_unregister_source(COM_UTIL_TRACER_DEFAULT_PROVIDER_NAME);
    return report_status(status, "削除");
}

/* Doxygen コメントは、ヘッダーに記載 */

int eventlog_register_run(int argc, char *argv[])
{
    if (argc < 2)
    {
        eventlog_register_print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "install") == 0)
    {
        return do_install();
    }
    if (strcmp(argv[1], "uninstall") == 0)
    {
        return do_uninstall();
    }

    fprintf(stderr, "不明なコマンド '%s'\n", argv[1]);
    eventlog_register_print_usage(argv[0]);
    return EXIT_FAILURE;
}

#else /* PLATFORM_LINUX */

/* Doxygen コメントは、ヘッダーに記載 */

int eventlog_register_run(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    fprintf(stderr, "eventlog-register は Windows 専用です。Linux ではイベント ログを使用しません。\n");
    return EXIT_FAILURE;
}

#endif /* PLATFORM_ */

/**
 *  @brief          メイン エントリ ポイント。
 *  @param[in]      argc  コマンド ライン引数の数。
 *  @param[in]      argv  コマンド ライン引数の配列。
 *  @return         正常終了時は 0、異常終了時は 0 以外を返す。
 */
int main(int argc, char *argv[])
{
    int run_result;
    int extract_rc;

    /* 昇格ワーカーとして再起動された場合、結果報告先フラグを argv から取り除く。
       引数解析より前に呼び出す。昇格ワーカーのコンソールは一切引き継がない
       (ensure_elevated() / report_status() がファイル経由で結果を受け渡す)。 */
    extract_rc = com_util_elevated_process_extract_result_target(&argc, argv);
#if defined(PLATFORM_WINDOWS)
    s_is_elevated_worker = extract_rc;
#else
    (void)extract_rc;
#endif

    com_util_console_init();
    run_result = eventlog_register_run(argc, argv);
    return run_result;
}
