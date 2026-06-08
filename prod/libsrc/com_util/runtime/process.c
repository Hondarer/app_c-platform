/**
 *******************************************************************************
 *  @file           process.c
 *  @brief          プロセス情報取得 実装。
 *  @author         Tetsuo Honda
 *  @date           2026/06/07
 *  @version        1.0.0
 *
 *  現在のプロセスの実行ファイル本体の絶対パスを取得します。
 *  - Linux  : readlink("/proc/self/exe") を使用します。
 *  - Windows: GetModuleFileNameW(NULL, ...) で取得し、UTF-8 に変換します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/runtime/process.h>
#include <com_util/crt/wchar_conv.h>
#include <com_util/crt/path.h>

#include <limits.h>
#include <stdlib.h>

#if defined(PLATFORM_LINUX)
    #include <sys/types.h>
    #include <unistd.h>
#elif defined(PLATFORM_WINDOWS)
    #include <shellapi.h>
    #pragma comment(lib, "Shell32.lib")
#endif /* PLATFORM_LINUX */

/* Doxygen コメントは、ヘッダーに記載 */

#if defined(PLATFORM_WINDOWS)
static int is_process_elevated(int *elevated)
{
    HANDLE token = NULL;
    TOKEN_ELEVATION elevation;
    DWORD returned_length;
    int rc;

    if (elevated == NULL)
    {
        return -1;
    }

    *elevated = 0;
    rc = -1;
    returned_length = 0;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &returned_length))
        {
            if (elevation.TokenIsElevated != 0)
            {
                *elevated = 1;
            }
            rc = 0;
        }
        CloseHandle(token);
    }

    return rc;
}

#endif /* PLATFORM_WINDOWS */

COM_UTIL_EXPORT int COM_UTIL_API com_util_process_get_executable_path(char *out_path, const size_t out_path_sz)
{
    if (out_path == NULL || out_path_sz == 0)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    {
        ssize_t len;

        len = readlink("/proc/self/exe", out_path, out_path_sz - 1);
        if (len < 0 || (size_t)len >= out_path_sz)
        {
            out_path[0] = '\0';
            return -1;
        }
        out_path[len] = '\0';
        return 0;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wbuf[PLATFORM_PATH_MAX];
        DWORD n;

        n = GetModuleFileNameW(NULL, wbuf, (DWORD)(sizeof(wbuf) / sizeof(wbuf[0])));
        if (n == 0 || n >= (DWORD)(sizeof(wbuf) / sizeof(wbuf[0])))
        {
            out_path[0] = '\0';
            return -1;
        }
        wbuf[n] = L'\0';
        if (com_util_wpath_to_utf8(out_path, out_path_sz, wbuf) < 0)
        {
            out_path[0] = '\0';
            return -1;
        }
        return 0;
    }
#else
    out_path[0] = '\0';
    return -1;
#endif /* PLATFORM_ */
}

COM_UTIL_EXPORT int COM_UTIL_API com_util_process_run_elevated_if_needed(const char *arguments, int *exit_code,
                                                                         int *handled)
{
    if (exit_code == NULL || handled == NULL)
    {
        return -1;
    }

    *exit_code = 0;
    *handled = 0;

#if defined(PLATFORM_WINDOWS)
    {
        SHELLEXECUTEINFOW exec_info;
        char exe_path[PLATFORM_PATH_MAX];
        wchar_t *wide_arguments = NULL;
        wchar_t *wide_exe_path;
        DWORD child_exit_code;
        int elevated;

        if (is_process_elevated(&elevated) != 0)
        {
            *exit_code = EXIT_FAILURE;
            return -1;
        }
        if (elevated != 0)
        {
            return 0;
        }

        if (com_util_process_get_executable_path(exe_path, sizeof(exe_path)) != 0)
        {
            *exit_code = EXIT_FAILURE;
            return -1;
        }

        wide_exe_path = com_util_utf8_to_wstr_alloc(exe_path);
        if (wide_exe_path == NULL)
        {
            *exit_code = EXIT_FAILURE;
            return -1;
        }
        if (arguments != NULL)
        {
            wide_arguments = com_util_utf8_to_wstr_alloc(arguments);
            if (wide_arguments == NULL)
            {
                free(wide_exe_path);
                *exit_code = EXIT_FAILURE;
                return -1;
            }
        }

        ZeroMemory(&exec_info, sizeof(exec_info));
        exec_info.cbSize = sizeof(exec_info);
        exec_info.fMask = SEE_MASK_NOCLOSEPROCESS;
        exec_info.hwnd = NULL;
        exec_info.lpVerb = L"runas";
        exec_info.lpFile = wide_exe_path;
        exec_info.lpParameters = wide_arguments;
        exec_info.nShow = SW_SHOWNORMAL;

        *handled = 1;

        if (!ShellExecuteExW(&exec_info))
        {
            free(wide_arguments);
            free(wide_exe_path);
            *exit_code = EXIT_FAILURE;
            return -1;
        }
        free(wide_arguments);
        free(wide_exe_path);

        WaitForSingleObject(exec_info.hProcess, INFINITE);
        child_exit_code = EXIT_FAILURE;
        if (!GetExitCodeProcess(exec_info.hProcess, &child_exit_code))
        {
            CloseHandle(exec_info.hProcess);
            *exit_code = EXIT_FAILURE;
            return -1;
        }
        CloseHandle(exec_info.hProcess);

        if (child_exit_code > INT_MAX)
        {
            *exit_code = EXIT_FAILURE;
            return -1;
        }
        *exit_code = (int)child_exit_code;
        return 0;
    }
#elif defined(PLATFORM_LINUX)
    if (geteuid() != 0)
    {
        (void)arguments;
        *exit_code = EXIT_FAILURE;
        return -1;
    }
    (void)arguments;
    return 0;
#else
    (void)arguments;
    *exit_code = EXIT_FAILURE;
    return -1;
#endif /* PLATFORM_ */
}
