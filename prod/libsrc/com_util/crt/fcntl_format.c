/**
 *******************************************************************************
 *  @file           fcntl_format.c
 *  @brief          書式指定パスに対応したファイル記述子オープン API を実装します。
 *
 *  共通のパス書式処理を使用して com_util_open() を呼び出します。
 *
 *******************************************************************************
 */

#include <com_util/crt/fcntl.h>
#include <com_util/crt/path.h>

#include <com_util/crt/path_format.h>
#include <com_util/base/error_internal.h>

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_vopen_fmt(const int flags, const int mode, com_util_error *detail_out, const char *format, va_list args)
{
    char filename[PLATFORM_PATH_MAX] = {0};
    int format_error;

    if (com_util_vformat_path(filename, sizeof(filename), format, args, &format_error) != 0)
    {
        (void)com_util_error_report_errno(detail_out, format_error);
        return -1;
    }

    return com_util_open(filename, flags, mode, detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_open_fmt(const int flags, const int mode, com_util_error *detail_out, const char *format, ...)
{
    int result;
    va_list args;

    va_start(args, format);
    result = com_util_vopen_fmt(flags, mode, detail_out, format, args);
    va_end(args);

    return result;
}
