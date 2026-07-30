/**
 *******************************************************************************
 *  @file           path_format.c
 *  @brief          書式指定された UTF-8 パスを生成する内部 API を実装します。
 *
 *  書式処理の結果を検証し、バッファー不足を errno 値へ変換します。
 *
 *******************************************************************************
 */

#include <com_util/crt/path_format.h>
#include <com_util/crt/stdio.h>

#include <errno.h>
#include <stdio.h>

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_vformat_path(char *path, const size_t path_size, const char *format, va_list args, int *error_out)
{
    int written;

    if (path == NULL || path_size == 0 || format == NULL)
    {
        if (error_out != NULL)
        {
            *error_out = EINVAL;
        }
        return -1;
    }

    written = vsnprintf(path, path_size, format, args);
    if (written < 0)
    {
        if (error_out != NULL)
        {
            *error_out = EINVAL;
        }
        return -1;
    }

    if (written >= (int)path_size)
    {
        if (error_out != NULL)
        {
            *error_out = ENAMETOOLONG;
        }
        return -1;
    }

    return 0;
}
