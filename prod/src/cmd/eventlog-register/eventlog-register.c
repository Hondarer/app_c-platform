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
 *  Linux ではイベント ログを使用しないため、案内を表示して終了します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include "eventlog-register.h"

#include <com_util/base/platform.h>
#include <com_util/console/console.h>
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
    #include <com_util/runtime/process.h>
    #include <com_util/trace/eventlog.h>

/**
 *  @brief          管理者権限を保証する。必要なら UAC 昇格して再実行する。
 *  @param[in]      command  昇格再実行するサブコマンド ("install" / "uninstall")。
 *  @param[out]     handled  昇格プロセスで処理済みの場合は 0 以外を格納する。
 *  @return         継続可能な場合は 0、失敗時または昇格プロセスの終了コードを返す。
 */
static int ensure_elevated(const char *command, int *handled)
{
    int exit_code;
    int rc;

    if (handled == NULL)
    {
        return EXIT_FAILURE;
    }

    exit_code = EXIT_FAILURE;
    rc = com_util_process_run_elevated_if_needed(command, &exit_code, handled);
    if (rc != 0)
    {
        fprintf(stderr, "管理者権限への昇格に失敗しました。\n");
        return EXIT_FAILURE;
    }

    if (*handled != 0)
    {
        return exit_code;
    }
    return 0;
}

/**
 *  @brief          イベント ソース API のステータスを表示し、終了コードに変換する。
 *  @param[in]      status  com_util_eventlog_register_source / unregister_source の戻り値。
 *  @param[in]      action  操作名 ("登録" / "削除")。
 *  @return         EXIT_SUCCESS / EXIT_FAILURE。
 */
static int report_status(const int status, const char *action)
{
    if (status == COM_UTIL_EVENTLOG_OK)
    {
        printf("イベント ソース '%s' を%sしました。\n", COM_UTIL_TRACER_DEFAULT_PROVIDER_NAME, action);
        return EXIT_SUCCESS;
    }
    if (status == COM_UTIL_EVENTLOG_ERR_ACCESS)
    {
        fprintf(stderr, "アクセスが拒否されました。管理者として実行してください。\n");
        return EXIT_FAILURE;
    }
    if (status == COM_UTIL_EVENTLOG_ERR_PARAM)
    {
        fprintf(stderr, "パラメーターが不正です。\n");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "システム エラーにより%sに失敗しました。\n", action);
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
    /* 昇格起動された場合、親コンソールへ再接続して出力を元のコンソールへ戻す。
       引き継ぎフラグを argv から取り除く必要があるため、引数解析より前に呼び出す。 */
    com_util_console_attach_parent(&argc, argv);

    com_util_console_init();

    return eventlog_register_run(argc, argv);
}
