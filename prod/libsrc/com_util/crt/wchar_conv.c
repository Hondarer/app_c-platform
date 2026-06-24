/**
 *******************************************************************************
 *  @file           wchar_conv.c
 *  @brief          UTF-8 / ワイド文字列変換 API 実装 (Windows 専用) です。
 *  @author         Tetsuo Honda
 *  @date           2026/06/08
 *  @version        1.0.0
 *
 *  Windows 上で UTF-8 文字列とワイド文字列 (UTF-16LE) を相互変換する
 *  関数を提供します。\n
 *  Linux では空の翻訳単位になります。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/crt/wchar_conv.h>

#if defined(PLATFORM_WINDOWS)

    #include <com_util/base/windows_sdk.h>
    #include <stdlib.h>

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_utf8_to_wpath(wchar_t *wbuf, size_t wbuf_count, const char *utf8_path)
{
    int n;

    if (utf8_path == NULL || wbuf == NULL || wbuf_count == 0)
    {
        return -1;
    }

    n = MultiByteToWideChar(CP_UTF8, 0, utf8_path, -1, wbuf, (int)wbuf_count);
    if (n <= 0)
    {
        return -1;
    }
    else
    {
        return n;
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_wpath_to_utf8(char *out, size_t out_size, const wchar_t *wpath)
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

/* Doxygen コメントは、ヘッダーに記載 */

wchar_t *com_util_utf8_to_wstr_alloc(const char *utf8_text)
{
    wchar_t *wtext;
    int wtext_count;

    if (utf8_text == NULL)
    {
        return NULL;
    }

    wtext_count = MultiByteToWideChar(CP_UTF8, 0, utf8_text, -1, NULL, 0);
    if (wtext_count <= 0)
    {
        return NULL;
    }

    wtext = (wchar_t *)malloc(sizeof(*wtext) * (size_t)wtext_count);
    if (wtext == NULL)
    {
        return NULL;
    }

    if (MultiByteToWideChar(CP_UTF8, 0, utf8_text, -1, wtext, wtext_count) <= 0)
    {
        free(wtext);
        return NULL;
    }

    return wtext;
}

/* Doxygen コメントは、ヘッダーに記載 */

char *com_util_wstr_to_utf8_alloc(const wchar_t *wtext)
{
    int utf8_count;
    char *utf8_text;

    if (wtext == NULL)
    {
        return NULL;
    }

    utf8_count = WideCharToMultiByte(CP_UTF8, 0, wtext, -1, NULL, 0, NULL, NULL);
    if (utf8_count <= 0)
    {
        return NULL;
    }

    utf8_text = (char *)malloc((size_t)utf8_count);
    if (utf8_text == NULL)
    {
        return NULL;
    }

    if (WideCharToMultiByte(CP_UTF8, 0, wtext, -1, utf8_text, utf8_count, NULL, NULL) <= 0)
    {
        free(utf8_text);
        return NULL;
    }

    return utf8_text;
}

#endif /* PLATFORM_WINDOWS */
