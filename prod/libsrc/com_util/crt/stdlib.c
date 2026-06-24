/**
 *******************************************************************************
 *  @file           stdlib.c
 *  @brief          stdlib 系の CRT 関数を抽象化する API を実装します。
 *
 *  環境変数の有無とバッファー不足を共通の戻り値へ変換します。
 *
 *******************************************************************************
 */

#include <com_util/crt/stdlib.h>
#include <com_util/base/platform.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_getenv(const char *name, char *buf, const size_t buf_size)
{
    if (name == NULL)
    {
        return EINVAL;
    }

#if defined(PLATFORM_LINUX)
    {
        const char *val = getenv(name);
        if (val == NULL)
        {
            return -1;
        }
        if (buf != NULL)
        {
            size_t len = strlen(val);
            if (len + 1 > buf_size)
            {
                return ERANGE;
            }
            memcpy(buf, val, len + 1);
        }
        return 0;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        char *val = NULL;
        size_t val_len = 0;

        if (_dupenv_s(&val, &val_len, name) != 0 || val == NULL)
        {
            return -1;
        }
        if (buf != NULL)
        {
            if (val_len > buf_size)
            {
                free(val);
                return ERANGE;
            }
            memcpy(buf, val, val_len);
        }
        free(val);
        return 0;
    }
#endif /* PLATFORM_ */
}
