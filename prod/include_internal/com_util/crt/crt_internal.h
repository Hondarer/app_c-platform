#ifndef COM_UTIL_CRT_INTERNAL_H
#define COM_UTIL_CRT_INTERNAL_H

#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)
    #include <com_util/base/windows_sdk.h>
    #include <wchar.h>

static int com_util_utf8_to_wpath(wchar_t *wbuf, size_t wbuf_count, const char *utf8_path)
{
    int n;

    if (utf8_path == NULL || wbuf == NULL || wbuf_count == 0)
    {
        return -1;
    }

    n = MultiByteToWideChar(CP_UTF8, 0, utf8_path, -1, wbuf, (int)wbuf_count);
    return (n <= 0) ? -1 : n;
}

static int com_util_wpath_to_utf8(char *out, size_t out_size, const wchar_t *wpath)
{
    int n;

    if (out == NULL || out_size == 0 || wpath == NULL)
    {
        return -1;
    }

    n = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, out, (int)out_size, NULL, NULL);
    if (n <= 0)
    {
        return -1;
    }

    /* Windows API が返す '\\' を '/' に正規化する */
    for (char *p = out; *p != '\0'; ++p)
    {
        if (*p == '\\')
        {
            *p = '/';
        }
    }

    return n;
}
#endif /* PLATFORM_WINDOWS */

#endif /* COM_UTIL_CRT_INTERNAL_H */
