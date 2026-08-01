/**
 *******************************************************************************
 *  @file           stdio_format.c
 *  @brief          書式指定パスに対応した stdio API を実装します。
 *
 *  共通のパス書式処理を使用してファイルのオープンと削除を行います。
 *
 *******************************************************************************
 */

#include <com_util/crt/stdio.h>
#include <com_util/crt/path.h>

#include <com_util/crt/path_format.h>
#include <com_util/base/error_internal.h>

#include <errno.h>

/* Doxygen コメントは、ヘッダーに記載 */

FILE *com_util_vfopen_fmt(const char *modes, com_util_error *detail_out, const char *format, va_list args)
{
    char filename[PLATFORM_PATH_MAX] = {0};
    int format_error;

    if (modes == NULL)
    {
        (void)com_util_error_report_errno(detail_out, EINVAL);
        return NULL;
    }

    if (com_util_vformat_path(filename, sizeof(filename), format, args, &format_error) != 0)
    {
        (void)com_util_error_report_errno(detail_out, format_error);
        return NULL;
    }

    return com_util_fopen(filename, modes, detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

FILE *com_util_fopen_fmt(const char *modes, com_util_error *detail_out, const char *format, ...)
{
    FILE *result;
    va_list args;

    va_start(args, format);
    result = com_util_vfopen_fmt(modes, detail_out, format, args);
    va_end(args);

    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_vremove_fmt(com_util_error *detail_out, const char *format, va_list args)
{
    char filename[PLATFORM_PATH_MAX] = {0};
    int format_error;

    if (com_util_vformat_path(filename, sizeof(filename), format, args, &format_error) != 0)
    {
        (void)com_util_error_report_errno(detail_out, format_error);
        return -1;
    }

    return com_util_remove(filename, detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_remove_fmt(com_util_error *detail_out, const char *format, ...)
{
    int result;
    va_list args;

    va_start(args, format);
    result = com_util_vremove_fmt(detail_out, format, args);
    va_end(args);

    return result;
}
