/**
 *******************************************************************************
 *  @file           library_api.c
 *  @brief          ライブラリ ロード系 Win32 API UTF-8 ラッパー実装。
 *  @author         Tetsuo Honda
 *  @date           2026/06/09
 *  @version        1.0.0
 *
 *  LoadLibraryU を実装します。\n
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

COM_UTIL_EXPORT HMODULE COM_UTIL_API LoadLibraryU(const char *utf8_file_name)
{
    HMODULE result;
    wchar_t *wfile_name;

    wfile_name = com_util_utf8_to_wstr_alloc(utf8_file_name);
    if (wfile_name == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    result = LoadLibraryW(wfile_name);
    free(wfile_name);
    return result;
}

#endif /* PLATFORM_WINDOWS */
