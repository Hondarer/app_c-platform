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

#include <com_util/crt/path_format.h>

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_vstat_fmt(com_util_file_stat_t *buf, const char *format, va_list args)
{
    char filename[PLATFORM_PATH_MAX] = {0};

    if (buf == NULL)
    {
        return -1;
    }

    if (com_util_vformat_path(filename, sizeof(filename), format, args, NULL) != 0)
    {
        return -1;
    }

    return com_util_stat(buf, filename);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_stat_fmt(com_util_file_stat_t *buf, const char *format, ...)
{
    int result;
    va_list args;

    va_start(args, format);
    result = com_util_vstat_fmt(buf, format, args);
    va_end(args);

    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_vmkdir_fmt(const char *format, va_list args)
{
    char filename[PLATFORM_PATH_MAX] = {0};

    if (com_util_vformat_path(filename, sizeof(filename), format, args, NULL) != 0)
    {
        return -1;
    }

    return com_util_mkdir(filename);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_mkdir_fmt(const char *format, ...)
{
    int result;
    va_list args;

    va_start(args, format);
    result = com_util_vmkdir_fmt(format, args);
    va_end(args);

    return result;
}
