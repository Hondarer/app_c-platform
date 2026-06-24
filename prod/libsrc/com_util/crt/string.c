/**
 *******************************************************************************
 *  @file           string.c
 *  @brief          string 系の CRT 関数を抽象化する API を実装します。
 *
 *  境界検証付き文字列操作と書式入力のプラットフォーム差異を吸収します。
 *
 *******************************************************************************
 */

#include <com_util/crt/string.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_strcpy(char *dest, const size_t dest_size, const char *src)
{
    size_t len;

    if (dest == NULL || dest_size == 0 || src == NULL)
    {
        return EINVAL;
    }

    len = strlen(src);
    if (len + 1 > dest_size)
    {
        dest[0] = '\0';
        return ERANGE;
    }

    memcpy(dest, src, len + 1);
    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_strncpy(char *dest, const size_t dest_size, const char *src, const size_t count)
{
    size_t len;

    if (dest == NULL || dest_size == 0 || src == NULL)
    {
        return EINVAL;
    }

    len = strlen(src);
    if (len > count)
    {
        len = count;
    }
    if (len >= dest_size)
    {
        len = dest_size - 1;
    }

    memcpy(dest, src, len);
    dest[len] = '\0';
    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_strcat(char *dest, const size_t dest_size, const char *src)
{
    size_t dest_len;
    size_t src_len;

    if (dest == NULL || dest_size == 0 || src == NULL)
    {
        return EINVAL;
    }

    dest_len = 0;
    while (dest_len < dest_size && dest[dest_len] != '\0')
    {
        dest_len++;
    }
    if (dest_len >= dest_size)
    {
        dest[0] = '\0';
        return ERANGE;
    }

    src_len = strlen(src);
    if (dest_len + src_len + 1 > dest_size)
    {
        return ERANGE;
    }

    memcpy(dest + dest_len, src, src_len + 1);
    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_wcscpy(wchar_t *dest, const size_t dest_size, const wchar_t *src)
{
    size_t len;

    if (dest == NULL || dest_size == 0 || src == NULL)
    {
        return EINVAL;
    }

    len = wcslen(src);
    if (len + 1 > dest_size)
    {
        dest[0] = L'\0';
        return ERANGE;
    }

    wmemcpy(dest, src, len + 1);
    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_vsscanf(const char *buffer, const char *format, va_list args)
{
    return vsscanf(buffer, format, args);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_sscanf(const char *buffer, const char *format, ...)
{
    va_list args;
    int result;

    va_start(args, format);
    result = com_util_vsscanf(buffer, format, args);
    va_end(args);
    return result;
}
