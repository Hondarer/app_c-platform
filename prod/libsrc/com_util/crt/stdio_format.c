/**
 *******************************************************************************
 *  @file           stdio_format.c
 *  @brief          書式指定パス対応の stdio API の実装です。
 *
 *  共通のパス書式処理を使用してファイルのオープンと削除を行います。
 *
 *******************************************************************************
 */

#include <com_util/crt/stdio.h>
#include <com_util/crt/path.h>

#include <com_util/crt/path_format.h>

#include <errno.h>

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT FILE *COM_UTIL_API com_util_vfopen_fmt(const char *modes,
                                                        int        *errno_out,
                                                        const char *format,
                                                        va_list     args)
{
    char filename[PLATFORM_PATH_MAX] = {0};
    int  format_error;

    if (modes == NULL)
    {
        if (errno_out != NULL)
        {
            *errno_out = EINVAL;
        }
        return NULL;
    }

    if (com_util_vformat_path(filename, sizeof(filename), format, args, &format_error) != 0)
    {
        if (errno_out != NULL)
        {
            *errno_out = format_error;
        }
        return NULL;
    }

    return com_util_fopen(filename, modes, errno_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT FILE *COM_UTIL_API com_util_fopen_fmt(const char *modes,
                                                       int        *errno_out,
                                                       const char *format,
                                                       ...)
{
    FILE   *result;
    va_list args;

    va_start(args, format);
    result = com_util_vfopen_fmt(modes, errno_out, format, args);
    va_end(args);

    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_vremove_fmt(const char *format,
                                                       va_list     args)
{
    char filename[PLATFORM_PATH_MAX] = {0};

    if (com_util_vformat_path(filename, sizeof(filename), format, args, NULL) != 0)
    {
        return -1;
    }

    return com_util_remove(filename);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_remove_fmt(const char *format, ...)
{
    int     result;
    va_list args;

    va_start(args, format);
    result = com_util_vremove_fmt(format, args);
    va_end(args);

    return result;
}
