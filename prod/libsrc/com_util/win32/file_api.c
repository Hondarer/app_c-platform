/**
 *******************************************************************************
 *  @file           file_api.c
 *  @brief          ファイル / パイプ系 Win32 API UTF-8 ラッパー実装です。
 *  @author         Tetsuo Honda
 *  @date           2026/06/09
 *  @version        1.0.0
 *
 *  CreateFileU / CreateNamedPipeU / GetModuleFileNameU を実装します。\n
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
    #include <string.h>

/* Doxygen コメントは、ヘッダーに記載 */

HANDLE CreateFileU(const char *utf8_path, DWORD desired_access, DWORD share_mode,
                   LPSECURITY_ATTRIBUTES security_attributes, DWORD creation_disposition, DWORD flags_and_attributes,
                   HANDLE template_file)
{
    HANDLE result;
    wchar_t *wpath;

    wpath = com_util_utf8_to_wstr_alloc(utf8_path);
    if (wpath == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    result = CreateFileW(wpath, desired_access, share_mode, security_attributes, creation_disposition,
                         flags_and_attributes, template_file);
    free(wpath);
    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

HANDLE CreateNamedPipeU(const char *utf8_name, DWORD open_mode, DWORD pipe_mode, DWORD max_instances,
                        DWORD out_buffer_size, DWORD in_buffer_size, DWORD default_timeout,
                        LPSECURITY_ATTRIBUTES security_attributes)
{
    HANDLE result;
    wchar_t *wname;

    wname = com_util_utf8_to_wstr_alloc(utf8_name);
    if (wname == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    result = CreateNamedPipeW(wname, open_mode, pipe_mode, max_instances, out_buffer_size, in_buffer_size,
                              default_timeout, security_attributes);
    free(wname);
    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

DWORD GetModuleFileNameU(HMODULE module, char *utf8_buf, DWORD size)
{
    wchar_t wbuf[32768]; /* 拡張パスの最大長に合わせた固定バッファー */
    DWORD wlen;
    char *utf8;
    size_t utf8_len;
    DWORD written;

    if (utf8_buf == NULL || size == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    wlen = GetModuleFileNameW(module, wbuf, (DWORD)(sizeof(wbuf) / sizeof(wbuf[0])));
    if (wlen == 0)
    {
        return 0;
    }

    /* パス区切り文字の正規化は行わない (wstr_to_utf8 を使用) */
    utf8 = com_util_wstr_to_utf8_alloc(wbuf);
    if (utf8 == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return 0;
    }

    utf8_len = strlen(utf8);
    if (utf8_len >= (size_t)size)
    {
        /* バッファー不足: 切り詰めて NUL 終端する */
        memcpy(utf8_buf, utf8, (size_t)(size - 1U));
        utf8_buf[size - 1U] = '\0';
        free(utf8);
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return size - 1U;
    }

    memcpy(utf8_buf, utf8, utf8_len + 1U);
    written = (DWORD)utf8_len;
    free(utf8);
    return written;
}

#endif /* PLATFORM_WINDOWS */
