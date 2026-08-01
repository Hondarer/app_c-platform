/**
 *******************************************************************************
 *  @file           sys_stat_format.c
 *  @brief          書式指定パスに対応したファイル情報 API を実装します。
 *
 *  共通のパス書式処理を使用して stat と mkdir の抽象 API を呼び出します。
 *
 *******************************************************************************
 */

#include <com_util/crt/sys/stat.h>
#include <com_util/crt/path.h>

#include <com_util/base/result.h>
#include <com_util/base/error_internal.h>
#include <com_util/crt/path_format.h>

#include <errno.h>

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_vstat_fmt(com_util_file_stat_t *buf, com_util_error *detail_out, const char *format, va_list args)
{
    char filename[PLATFORM_PATH_MAX] = {0};
    int format_error;

    if (buf == NULL)
    {
        return com_util_error_report_errno(detail_out, EINVAL);
    }

    if (com_util_vformat_path(filename, sizeof(filename), format, args, &format_error) != 0)
    {
        return com_util_error_report_errno(detail_out, format_error);
    }

    return com_util_stat(buf, detail_out, filename);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_stat_fmt(com_util_file_stat_t *buf, com_util_error *detail_out, const char *format, ...)
{
    int result;
    va_list args;

    va_start(args, format);
    result = com_util_vstat_fmt(buf, detail_out, format, args);
    va_end(args);

    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_vmkdir_fmt(com_util_error *detail_out, const char *format, va_list args)
{
    char filename[PLATFORM_PATH_MAX] = {0};
    int format_error;

    if (com_util_vformat_path(filename, sizeof(filename), format, args, &format_error) != 0)
    {
        return com_util_error_report_errno(detail_out, format_error);
    }

    return com_util_mkdir(filename, detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_mkdir_fmt(com_util_error *detail_out, const char *format, ...)
{
    int result;
    va_list args;

    va_start(args, format);
    result = com_util_vmkdir_fmt(detail_out, format, args);
    va_end(args);

    return result;
}
