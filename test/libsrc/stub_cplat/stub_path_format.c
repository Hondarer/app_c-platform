/**
 *  @file           stub_path_format.c
 *  @brief          書式付きパス生成の内部 API を、呼び出し元の単体テスト向けに提供します。
 *
 *  path_format.c のカバレッジは pathFormatTest が担う。
 *  本スタブは引数検査と cplat_vsnprintf への委譲だけを行い、書式展開の成否を呼び出し元へ返す。
 */

#include <cplat/crt/path_format.h>
#include <cplat/crt/stdio.h>

#include <cplat/base/result.h>

#include <errno.h>

int cplat_vformat_path(char *path, const size_t path_size, const char *format, va_list args, int *error_out)
{
    int ret;

    if (path == NULL || path_size == 0 || format == NULL)
    {
        if (error_out != NULL)
        {
            *error_out = EINVAL;
        }
        return -1;
    }

    ret = cplat_vsnprintf(path, path_size, format, args);
    if (ret == CPLAT_ERR_BUFFER_TOO_SMALL)
    {
        if (error_out != NULL)
        {
            *error_out = ENAMETOOLONG;
        }
        return -1;
    }
    if (ret != CPLAT_OK)
    {
        if (error_out != NULL)
        {
            *error_out = EINVAL;
        }
        return -1;
    }

    return 0;
}
