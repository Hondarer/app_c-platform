/**
 *******************************************************************************
 *  @file           process_api.c
 *  @brief          プロセス系 Win32 API UTF-8 ラッパー実装です。
 *  @author         Tetsuo Honda
 *  @date           2026/06/09
 *  @version        1.0.0
 *
 *  CreateProcessU を実装します。\n
 *  Linux では空の翻訳単位になります。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/win32/win32.h>

#if defined(PLATFORM_WINDOWS)

    #include <com_util/crt/wchar_conv.h>
    #include <stdlib.h>

/* Doxygen コメントは、ヘッダーに記載 */

BOOL CreateProcessU(const char *utf8_application_name, const char *utf8_command_line,
                    LPSECURITY_ATTRIBUTES process_attributes, LPSECURITY_ATTRIBUTES thread_attributes,
                    BOOL inherit_handles, DWORD creation_flags, LPVOID environment, const char *utf8_current_directory,
                    LPSTARTUPINFOW startup_info, LPPROCESS_INFORMATION process_information)
{
    BOOL result;
    wchar_t *wapp = NULL;
    wchar_t *wcmdline = NULL; /* CreateProcessW が書き換え可能なバッファーを要求するため確保する */
    wchar_t *wcurdir = NULL;

    if (utf8_application_name != NULL)
    {
        wapp = com_util_utf8_to_wstr_alloc(utf8_application_name);
    }
    if (utf8_command_line != NULL)
    {
        wcmdline = com_util_utf8_to_wstr_alloc(utf8_command_line);
    }
    if (utf8_current_directory != NULL)
    {
        wcurdir = com_util_utf8_to_wstr_alloc(utf8_current_directory);
    }

    if ((utf8_application_name != NULL && wapp == NULL) || (utf8_command_line != NULL && wcmdline == NULL) ||
        (utf8_current_directory != NULL && wcurdir == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        free(wapp);
        free(wcmdline);
        free(wcurdir);
        return FALSE;
    }

    result = CreateProcessW(wapp, wcmdline, process_attributes, thread_attributes, inherit_handles, creation_flags,
                            environment, wcurdir, startup_info, process_information);
    free(wapp);
    free(wcmdline);
    free(wcurdir);
    return result;
}

#endif /* PLATFORM_WINDOWS */
