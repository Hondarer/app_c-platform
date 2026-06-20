/**
 *******************************************************************************
 *  @file           unistd_format.c
 *  @brief          書式指定パス対応のアクセス確認 API の実装です。
 *
 *  共通のパス書式処理を使用して com_util_access() を呼び出します。
 *
 *******************************************************************************
 */

#include <com_util/crt/unistd.h>
#include <com_util/crt/path.h>

#include <com_util/crt/path_format.h>

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_vaccess_fmt(const int mode, const char *format, va_list args)
{
    char filename[PLATFORM_PATH_MAX] = {0};

    if (com_util_vformat_path(filename, sizeof(filename), format, args, NULL) != 0)
    {
        return -1;
    }

    return com_util_access(filename, mode);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_access_fmt(const int mode, const char *format, ...)
{
    int result;
    va_list args;

    va_start(args, format);
    result = com_util_vaccess_fmt(mode, format, args);
    va_end(args);

    return result;
}
