/**
 *******************************************************************************
 *  @file           elevated_process.c
 *  @brief          管理者権限の確認と昇格プロセスの起動を実装します。
 *  @author         Tetsuo Honda
 *  @date           2026/06/20
 *  @version        1.0.0
 *
 *  管理者/root 権限の確認と、必要に応じた昇格プロセスの起動を提供します。
 *  プロセスの待機・終了コード取得・破棄はプロセス起動ユーティリティ (process.c) を
 *  再利用します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/base/result.h>
#include <com_util/crt/stdio.h>
#include <com_util/runtime/elevated_process.h>
#include <com_util/runtime/process.h>
#include <com_util/runtime/process_internal.h>
#include <com_util/console/console_internal.h>
#include <com_util/crt/path.h>
#include <com_util/crt/wchar_conv.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(PLATFORM_LINUX)
    #include <unistd.h>
#elif defined(PLATFORM_WINDOWS)
    #include <com_util/base/windows_sdk.h>
    #include <shellapi.h>
    #include <wchar.h>
    #pragma comment(lib, "Shell32.lib")

/**
 *  @brief          昇格プロセスへ結果報告先の一時ファイル パスを引き継ぐ内部フラグです。
 *
 *  com_util_elevated_process_run_with_result() が昇格プロセスのコマンド ラインへ
 *  `{FLAG}={一時ファイルの UTF-8 パス}` の形式で付与し、
 *  com_util_elevated_process_extract_result_target() がこれを検出して報告先を保持します。
 */
    #define COM_UTIL_PROCESS_RESULT_TARGET_FLAG "--com-util-result-file"

static char s_result_target_path[PLATFORM_PATH_MAX] = {0};

/**
 *  @brief          現在のプロセスが対話セッションで動作しているかを確認します。
 *
 *  Windows のサービス プロセスはセッション 0 に隔離され、UAC (User Account Control)
 *  ダイアログを表示できる対話デスクトップを持ちません。セッション 0 で未昇格のまま
 *  `ShellExecuteExW` の `runas` verb を呼び出すと、対話デスクトップが無いために
 *  昇格が失敗または応答不能になるおそれがあるため、事前にセッション ID で判定します。
 *  see: https://learn.microsoft.com/windows/win32/services/interactive-services
 *
 *  @return         対話セッションの場合は非 0、非対話セッション (セッション 0) の場合は 0、
 *                  判定に失敗した場合は -1 を返します。
 */
static int is_current_session_interactive(void)
{
    DWORD session_id = 0;

    if (!ProcessIdToSessionId(GetCurrentProcessId(), &session_id))
    {
        return -1;
    }
    return (session_id != 0);
}
#endif /* PLATFORM_ */

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_elevated_process_is_elevated(int *elevated)
{
    if (elevated == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    *elevated = 0;

#if defined(PLATFORM_LINUX)
    if (geteuid() == 0)
    {
        *elevated = 1;
    }
    return COM_UTIL_OK;
#elif defined(PLATFORM_WINDOWS)
    {
        HANDLE token = NULL;
        TOKEN_ELEVATION elevation;
        DWORD returned_length;
        int rc;

        rc = COM_UTIL_ERR_UNKNOWN;
        returned_length = 0;

        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        {
            if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &returned_length))
            {
                if (elevation.TokenIsElevated != 0)
                {
                    *elevated = 1;
                }
                rc = COM_UTIL_OK;
            }
            CloseHandle(token);
        }

        return rc;
    }
#else
    return COM_UTIL_ERR_UNSUPPORTED;
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_elevated_process_run_if_needed(const char *arguments, int *exit_code, int *handled)
{
    if (exit_code == NULL || handled == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    *exit_code = 0;
    *handled = 0;

#if defined(PLATFORM_WINDOWS)
    {
        SHELLEXECUTEINFOW exec_info;
        char exe_path[PLATFORM_PATH_MAX];
        char *combined_arguments = NULL;
        const char *effective_arguments;
        wchar_t *wide_arguments = NULL;
        wchar_t *wide_exe_path;
        com_util_process *child_process;
        int child_exit_code;
        int elevated;
        int inherit_console;
        int diag_enabled;

        if (com_util_elevated_process_is_elevated(&elevated) != COM_UTIL_OK)
        {
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_UNKNOWN;
        }
        if (elevated != 0)
        {
            return COM_UTIL_OK;
        }

        /* セッション 0 (非対話セッション) では UAC ダイアログを表示できないため、
           昇格を試みず即座に失敗させる。 */
        if (is_current_session_interactive() == 0)
        {
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_UNKNOWN;
        }

        if (com_util_process_get_executable_path(exe_path, sizeof(exe_path)) != COM_UTIL_OK)
        {
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_UNKNOWN;
        }

        /* 親にコンソールがある場合のみ、昇格プロセスへ親コンソールを引き継ぐ。
           昇格プロセス側は com_util_console_attach_parent() で再接続する。 */
        inherit_console = (GetConsoleWindow() != NULL);
        diag_enabled = 0;
        if (GetEnvironmentVariableA(COM_UTIL_CONSOLE_ATTACH_DIAG_ENV, NULL, 0) > 0)
        {
            diag_enabled = 1;
        }

        effective_arguments = arguments;
        if (inherit_console != 0 || diag_enabled != 0)
        {
            DWORD parent_pid;
            unsigned long long parent_console_window;
            size_t arg_len;
            size_t buf_sz;
            int offset;
            int written;

            parent_pid = 0;
            /* 親コンソールの window ハンドルを引き継ぎ、子側が一時コンソールではなく
               親コンソールへ確実に再接続できたかを確認できるようにする。 */
            parent_console_window = 0;
            arg_len = 0;
            if (arguments != NULL)
            {
                arg_len = strlen(arguments);
            }
            /* 区切り空白 + フラグ + '=' + PID (最大 10 桁) + ':' + HWND (最大 20 桁) + 終端の余裕を確保する */
            buf_sz = arg_len + 1;
            if (diag_enabled != 0)
            {
                buf_sz += strlen(COM_UTIL_CONSOLE_ATTACH_DIAG_FLAG) + 1;
            }
            if (inherit_console != 0)
            {
                parent_pid = GetCurrentProcessId();
                parent_console_window = (unsigned long long)(uintptr_t)GetConsoleWindow();
                buf_sz += strlen(COM_UTIL_CONSOLE_HANDOVER_FLAG) + 49;
            }
            combined_arguments = (char *)malloc(buf_sz);
            if (combined_arguments == NULL)
            {
                *exit_code = EXIT_FAILURE;
                return COM_UTIL_ERR_OUT_OF_MEMORY;
            }
            offset = 0;
            if (arg_len > 0)
            {
                offset = snprintf(combined_arguments, buf_sz, "%s", arguments);
                if (offset < 0 || (size_t)offset >= buf_sz)
                {
                    free(combined_arguments);
                    *exit_code = EXIT_FAILURE;
                    return COM_UTIL_ERR_UNKNOWN;
                }
            }
            else
            {
                combined_arguments[0] = '\0';
            }
            if (diag_enabled != 0)
            {
                const char *separator;

                separator = "";
                if (offset > 0)
                {
                    separator = " ";
                }
                written = snprintf(combined_arguments + offset, buf_sz - (size_t)offset, "%s%s", separator,
                                   COM_UTIL_CONSOLE_ATTACH_DIAG_FLAG);
                if (written < 0 || (size_t)written >= buf_sz - (size_t)offset)
                {
                    free(combined_arguments);
                    *exit_code = EXIT_FAILURE;
                    return COM_UTIL_ERR_UNKNOWN;
                }
                offset += written;
            }
            if (inherit_console != 0)
            {
                const char *separator;

                separator = "";
                if (offset > 0)
                {
                    separator = " ";
                }
                written = snprintf(combined_arguments + offset, buf_sz - (size_t)offset, "%s%s=%lu:%llu", separator,
                                   COM_UTIL_CONSOLE_HANDOVER_FLAG, (unsigned long)parent_pid, parent_console_window);
                if (written < 0 || (size_t)written >= buf_sz - (size_t)offset)
                {
                    free(combined_arguments);
                    *exit_code = EXIT_FAILURE;
                    return COM_UTIL_ERR_UNKNOWN;
                }
                offset += written;
            }
            effective_arguments = combined_arguments;
        }

        wide_exe_path = com_util_utf8_to_wstr_alloc(exe_path);
        if (wide_exe_path == NULL)
        {
            free(combined_arguments);
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_OUT_OF_MEMORY;
        }
        if (effective_arguments != NULL)
        {
            wide_arguments = com_util_utf8_to_wstr_alloc(effective_arguments);
            if (wide_arguments == NULL)
            {
                free(combined_arguments);
                free(wide_exe_path);
                *exit_code = EXIT_FAILURE;
                return COM_UTIL_ERR_OUT_OF_MEMORY;
            }
        }
        free(combined_arguments);
        combined_arguments = NULL;

        ZeroMemory(&exec_info, sizeof(exec_info));
        exec_info.cbSize = sizeof(exec_info);
        exec_info.fMask = SEE_MASK_NOCLOSEPROCESS;
        exec_info.hwnd = NULL;
        exec_info.lpVerb = L"runas";
        exec_info.lpFile = wide_exe_path;
        exec_info.lpParameters = wide_arguments;
        /* コンソール引き継ぎ時は昇格プロセスの一時コンソールを隠す。
           引き継がない場合は従来どおり通常表示とする。 */
        if (inherit_console != 0)
        {
            exec_info.nShow = SW_HIDE;
        }
        else
        {
            exec_info.nShow = SW_SHOWNORMAL;
        }

        *handled = 1;

        if (!ShellExecuteExW(&exec_info))
        {
            free(wide_arguments);
            free(wide_exe_path);
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_UNKNOWN;
        }
        free(wide_arguments);
        free(wide_exe_path);

        child_process = com_util_process_adopt_native((intptr_t)exec_info.hProcess);
        if (child_process == NULL)
        {
            CloseHandle(exec_info.hProcess);
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_UNKNOWN;
        }
        if (com_util_process_wait(child_process, COM_UTIL_PROCESS_WAIT_FOREVER) != COM_UTIL_OK ||
            com_util_process_get_exit_code(child_process, &child_exit_code) != COM_UTIL_OK)
        {
            com_util_process_destroy(child_process);
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_UNKNOWN;
        }
        com_util_process_destroy(child_process);

        *exit_code = child_exit_code;
        return COM_UTIL_OK;
    }
#elif defined(PLATFORM_LINUX)
    {
        int elevated;

        (void)arguments;
        if (com_util_elevated_process_is_elevated(&elevated) != COM_UTIL_OK || elevated == 0)
        {
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_UNKNOWN;
        }
        return COM_UTIL_OK;
    }
#else
    (void)arguments;
    *exit_code = EXIT_FAILURE;
    return COM_UTIL_ERR_UNSUPPORTED;
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_elevated_process_run_with_result(const char *arguments, int *exit_code, int *handled, char *result_message,
                                              size_t result_message_size)
{
    if (exit_code == NULL || handled == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    *exit_code = 0;
    *handled = 0;
    if (result_message != NULL && result_message_size > 0)
    {
        result_message[0] = '\0';
    }

#if defined(PLATFORM_WINDOWS)
    {
        SHELLEXECUTEINFOW exec_info;
        char exe_path[PLATFORM_PATH_MAX];
        char result_path[PLATFORM_PATH_MAX];
        wchar_t wide_temp_dir[PLATFORM_PATH_MAX];
        wchar_t wide_result_path[PLATFORM_PATH_MAX];
        char *combined_arguments;
        wchar_t *wide_arguments;
        wchar_t *wide_exe_path;
        com_util_process *child_process;
        int child_exit_code;
        int elevated;
        size_t arg_len;
        size_t buf_sz;
        DWORD temp_len;

        if (com_util_elevated_process_is_elevated(&elevated) != COM_UTIL_OK)
        {
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_UNKNOWN;
        }
        if (elevated != 0)
        {
            return COM_UTIL_OK;
        }

        /* セッション 0 (非対話セッション) では UAC ダイアログを表示できないため、
           昇格を試みず即座に失敗させる。 */
        if (is_current_session_interactive() == 0)
        {
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_UNKNOWN;
        }

        if (com_util_process_get_executable_path(exe_path, sizeof(exe_path)) != COM_UTIL_OK)
        {
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_UNKNOWN;
        }

        /* 昇格プロセスが結果メッセージを書き込む一時ファイルを確保する。GetTempFileNameW は
           一意な空ファイルを作成するため、並行する別プロセスとの衝突を避けられる。 */
        /* 本関数内の一時ファイル操作はワイドのまま完結するため、*U ラッパーを経由しない */
        temp_len = GetTempPathW((DWORD)(sizeof(wide_temp_dir) / sizeof(wide_temp_dir[0])), wide_temp_dir);
        if (temp_len == 0 || temp_len >= sizeof(wide_temp_dir) / sizeof(wide_temp_dir[0]))
        {
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_UNKNOWN;
        }
        if (GetTempFileNameW(wide_temp_dir, L"cur", 0, wide_result_path) == 0)
        {
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_UNKNOWN;
        }
        if (com_util_wpath_to_utf8(result_path, sizeof(result_path), wide_result_path) < 0)
        {
            DeleteFileW(wide_result_path);
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_UNKNOWN;
        }

        arg_len = 0;
        if (arguments != NULL)
        {
            arg_len = strlen(arguments);
        }
        /* 区切り空白 + フラグ + '=' + '"' + パス + '"' + 終端の余裕を確保する */
        buf_sz = arg_len + 1 + strlen(COM_UTIL_PROCESS_RESULT_TARGET_FLAG) + 1 + 1 + strlen(result_path) + 1 + 1;
        combined_arguments = (char *)malloc(buf_sz);
        if (combined_arguments == NULL)
        {
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_OUT_OF_MEMORY;
        }
        /* result_path はユーザー プロファイル配下 (ユーザー名に空白を含みうる) から生成されるため、
           クォートしないと CRT のコマンドライン分割で複数トークンとなり、
           com_util_elevated_process_extract_result_target() でのフラグ抽出に失敗する。
           クォート区間内の空白はトークン区切りにならず、クォート文字自体は結果の argv から除去される。
           see: https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments */
        if (arg_len > 0)
        {
            (void)snprintf(combined_arguments, buf_sz, "%s %s=\"%s\"", arguments, COM_UTIL_PROCESS_RESULT_TARGET_FLAG,
                           result_path);
        }
        else
        {
            (void)snprintf(combined_arguments, buf_sz, "%s=\"%s\"", COM_UTIL_PROCESS_RESULT_TARGET_FLAG, result_path);
        }

        wide_exe_path = com_util_utf8_to_wstr_alloc(exe_path);
        if (wide_exe_path == NULL)
        {
            free(combined_arguments);
            DeleteFileW(wide_result_path);
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_OUT_OF_MEMORY;
        }
        wide_arguments = com_util_utf8_to_wstr_alloc(combined_arguments);
        free(combined_arguments);
        if (wide_arguments == NULL)
        {
            free(wide_exe_path);
            DeleteFileW(wide_result_path);
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_OUT_OF_MEMORY;
        }

        ZeroMemory(&exec_info, sizeof(exec_info));
        exec_info.cbSize = sizeof(exec_info);
        exec_info.fMask = SEE_MASK_NOCLOSEPROCESS;
        exec_info.hwnd = NULL;
        exec_info.lpVerb = L"runas";
        exec_info.lpFile = wide_exe_path;
        exec_info.lpParameters = wide_arguments;
        /* コンソールを一切引き継がないため、昇格プロセスは常に非表示で起動する。 */
        exec_info.nShow = SW_HIDE;

        *handled = 1;

        if (!ShellExecuteExW(&exec_info))
        {
            free(wide_arguments);
            free(wide_exe_path);
            DeleteFileW(wide_result_path);
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_UNKNOWN;
        }
        free(wide_arguments);
        free(wide_exe_path);

        child_process = com_util_process_adopt_native((intptr_t)exec_info.hProcess);
        if (child_process == NULL)
        {
            CloseHandle(exec_info.hProcess);
            DeleteFileW(wide_result_path);
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_UNKNOWN;
        }
        if (com_util_process_wait(child_process, COM_UTIL_PROCESS_WAIT_FOREVER) != COM_UTIL_OK ||
            com_util_process_get_exit_code(child_process, &child_exit_code) != COM_UTIL_OK)
        {
            com_util_process_destroy(child_process);
            DeleteFileW(wide_result_path);
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_UNKNOWN;
        }
        com_util_process_destroy(child_process);

        if (result_message != NULL && result_message_size > 0)
        {
            HANDLE h;

            h = CreateFileW(wide_result_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                            NULL);
            if (h != INVALID_HANDLE_VALUE)
            {
                DWORD bytes_read = 0;
                DWORD to_read = (DWORD)(result_message_size - 1);

                if (ReadFile(h, result_message, to_read, &bytes_read, NULL))
                {
                    result_message[bytes_read] = '\0';
                }
                CloseHandle(h);
            }
        }
        DeleteFileW(wide_result_path);

        *exit_code = child_exit_code;
        return COM_UTIL_OK;
    }
#elif defined(PLATFORM_LINUX)
    {
        int elevated;

        /* 別プロセスを起動しないため、報告の余地がない。 */
        (void)result_message;
        (void)result_message_size;
        (void)arguments;
        if (com_util_elevated_process_is_elevated(&elevated) != COM_UTIL_OK || elevated == 0)
        {
            *exit_code = EXIT_FAILURE;
            return COM_UTIL_ERR_UNKNOWN;
        }
        return COM_UTIL_OK;
    }
#else
    (void)arguments;
    (void)result_message;
    (void)result_message_size;
    *exit_code = EXIT_FAILURE;
    return COM_UTIL_ERR_UNSUPPORTED;
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_elevated_process_extract_result_target(int *argc, char **argv, int *detected_out)
{
#if defined(PLATFORM_WINDOWS)
    const char *prefix = COM_UTIL_PROCESS_RESULT_TARGET_FLAG "=";
    size_t prefix_len;
    int n;
    int i;

    if (detected_out != NULL)
    {
        *detected_out = 0;
    }

    if (argc == NULL || argv == NULL)
    {
        return COM_UTIL_OK;
    }

    prefix_len = strlen(prefix);
    n = *argc;
    for (i = 1; i < n; i++)
    {
        int j;
        size_t value_len;

        if (argv[i] == NULL || strncmp(argv[i], prefix, prefix_len) != 0)
        {
            continue;
        }

        value_len = strlen(argv[i] + prefix_len);
        if (value_len >= sizeof(s_result_target_path))
        {
            value_len = sizeof(s_result_target_path) - 1;
        }
        memcpy(s_result_target_path, argv[i] + prefix_len, value_len);
        s_result_target_path[value_len] = '\0';

        for (j = i; j < n - 1; j++)
        {
            argv[j] = argv[j + 1];
        }
        argv[n - 1] = NULL;
        *argc = n - 1;
        if (detected_out != NULL)
        {
            *detected_out = 1;
        }
        return COM_UTIL_OK;
    }
    return COM_UTIL_OK;
#else
    (void)argc;
    (void)argv;
    if (detected_out != NULL)
    {
        *detected_out = 0;
    }
    return COM_UTIL_OK;
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_elevated_process_report_result(const char *message)
{
    if (message == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

#if defined(PLATFORM_WINDOWS)
    {
        wchar_t wide_path[PLATFORM_PATH_MAX];
        HANDLE h;
        DWORD written;
        size_t len;

        if (s_result_target_path[0] == '\0')
        {
            return COM_UTIL_ERR_UNKNOWN;
        }
        if (com_util_utf8_to_wpath(wide_path, sizeof(wide_path) / sizeof(wide_path[0]), s_result_target_path) < 0)
        {
            return COM_UTIL_ERR_UNKNOWN;
        }
        /* wide_path はワイド文字列であり UTF-8 が関与しないため CreateFileU を使わない */
        h = CreateFileW(wide_path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE)
        {
            return COM_UTIL_ERR_UNKNOWN;
        }
        len = strlen(message);
        if (!WriteFile(h, message, (DWORD)len, &written, NULL))
        {
            CloseHandle(h);
            return COM_UTIL_ERR_UNKNOWN;
        }
        CloseHandle(h);
        return COM_UTIL_OK;
    }
#else
    (void)message;
    return COM_UTIL_ERR_UNKNOWN;
#endif /* PLATFORM_ */
}
