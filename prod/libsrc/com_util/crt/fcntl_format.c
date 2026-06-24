/**
 *******************************************************************************
 *  @file           fcntl_format.c
 *  @brief          書式指定パス対応のファイル記述子オープン API の実装です。
 *
 *  共通のパス書式処理を使用して com_util_open() を呼び出します。
 *
 *******************************************************************************
 */

#include <com_util/crt/fcntl.h>
#include <com_util/crt/path.h>

#include <com_util/crt/path_format.h>

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_vopen_fmt(const int flags, const int mode, const char *format, va_list args)
{
    char filename[PLATFORM_PATH_MAX] = {0};

    if (com_util_vformat_path(filename, sizeof(filename), format, args, NULL) != 0)
    {
        return -1;
    }

    return com_util_open(filename, flags, mode);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_open_fmt(const int flags, const int mode, const char *format, ...)
{
    int result;
    va_list args;

    va_start(args, format);
    result = com_util_vopen_fmt(flags, mode, format, args);
    va_end(args);

    return result;
}
