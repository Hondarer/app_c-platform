/**
 *******************************************************************************
 *  @file           string.c
 *  @brief          string 系の CRT 関数を抽象化する API を実装します。
 *
 *  境界検証付き文字列操作と書式入力のプラットフォーム差異を吸収します。
 *
 *******************************************************************************
 */

#include <cplat/crt/string.h>

#include <stdio.h>
#include <string.h>

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_strcpy(char *dest, const size_t dest_size, const char *src)
{
    size_t len;

    if (dest == NULL || dest_size == 0 || src == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    len = strlen(src);
    if (len + 1 > dest_size)
    {
        dest[0] = '\0';
        return CPLAT_ERR_BUFFER_TOO_SMALL;
    }

    memcpy(dest, src, len + 1);
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_strncpy(char *dest, const size_t dest_size, const char *src, const size_t count)
{
    size_t len;

    if (dest == NULL || dest_size == 0 || src == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
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
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_strcat(char *dest, const size_t dest_size, const char *src)
{
    size_t dest_len;
    size_t src_len;

    if (dest == NULL || dest_size == 0 || src == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    dest_len = 0;
    while (dest_len < dest_size && dest[dest_len] != '\0')
    {
        dest_len++;
    }
    if (dest_len >= dest_size)
    {
        dest[0] = '\0';
        return CPLAT_ERR_BUFFER_TOO_SMALL;
    }

    src_len = strlen(src);
    if (dest_len + src_len + 1 > dest_size)
    {
        return CPLAT_ERR_BUFFER_TOO_SMALL;
    }

    memcpy(dest + dest_len, src, src_len + 1);
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_strncat(char *dest, const size_t dest_size, const char *src, const size_t count)
{
    size_t dest_len;
    size_t src_len;

    if (dest == NULL || dest_size == 0 || src == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    dest_len = 0;
    while (dest_len < dest_size && dest[dest_len] != '\0')
    {
        dest_len++;
    }
    if (dest_len >= dest_size)
    {
        dest[0] = '\0';
        return CPLAT_ERR_BUFFER_TOO_SMALL;
    }

    /* src が count 文字未満で終端している場合に備え、終端までの長さで頭打ちにする */
    src_len = 0;
    while (src_len < count && src[src_len] != '\0')
    {
        src_len++;
    }

    if (dest_len + src_len + 1 > dest_size)
    {
        return CPLAT_ERR_BUFFER_TOO_SMALL;
    }

    memcpy(dest + dest_len, src, src_len);
    dest[dest_len + src_len] = '\0';
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

char *cplat_strtok_r(char *str, const char *delim, char **saveptr)
{
    if (delim == NULL || saveptr == NULL)
    {
        return NULL;
    }

#if defined(PLATFORM_LINUX)
    return strtok_r(str, delim, saveptr);
#elif defined(PLATFORM_WINDOWS)
    /* Windows では再入可能版の名前が strtok_s である。引数と意味は strtok_r と同じ。
     * see: https://learn.microsoft.com/cpp/c-runtime-library/reference/strtok-s-strtok-s-l-wcstok-s-wcstok-s-l-mbstok-s-mbstok-s-l */
    return strtok_s(str, delim, saveptr);
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_wcscpy(wchar_t *dest, const size_t dest_size, const wchar_t *src)
{
    size_t len;

    if (dest == NULL || dest_size == 0 || src == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    len = wcslen(src);
    if (len + 1 > dest_size)
    {
        dest[0] = L'\0';
        return CPLAT_ERR_BUFFER_TOO_SMALL;
    }

    wmemcpy(dest, src, len + 1);
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_vsscanf(const char *buffer, const char *format, va_list args)
{
    return vsscanf(buffer, format, args);
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_sscanf(const char *buffer, const char *format, ...)
{
    va_list args;
    int result;

    va_start(args, format);
    result = cplat_vsscanf(buffer, format, args);
    va_end(args);
    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

char *cplat_strdup(const char *src)
{
    if (src == NULL)
    {
        return NULL;
    }

#if defined(PLATFORM_LINUX)
    return strdup(src);
#elif defined(PLATFORM_WINDOWS)
    /* MSVC では strdup が C4996 で非推奨のため _strdup を使用する。
     * see: https://learn.microsoft.com/cpp/c-runtime-library/reference/strdup-wcsdup-mbsdup */
    return _strdup(src);
#endif /* PLATFORM_ */
}
