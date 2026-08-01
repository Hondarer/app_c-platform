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
#include <com_util/base/error_internal.h>
#include <com_util/base/platform.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_getenv(const char *name, char *buf, const size_t buf_size, int *exists_out, com_util_error *detail_out)
{
    if (exists_out != NULL)
    {
        *exists_out = 0;
    }
    if (name == NULL)
    {
        return com_util_error_report_errno(detail_out, EINVAL);
    }

#if defined(PLATFORM_LINUX)
    {
        const char *val = getenv(name);
        if (val == NULL)
        {
            if (buf != NULL && buf_size > 0)
            {
                buf[0] = '\0';
            }
            return com_util_error_report_success(detail_out);
        }
        if (exists_out != NULL)
        {
            *exists_out = 1;
        }
        if (buf != NULL)
        {
            size_t len = strlen(val);
            if (len + 1 > buf_size)
            {
                return com_util_error_report_errno(detail_out, ERANGE);
            }
            memcpy(buf, val, len + 1);
        }
        return com_util_error_report_success(detail_out);
    }
#elif defined(PLATFORM_WINDOWS)
    {
        char *val = NULL;
        size_t val_len = 0;
        const errno_t err = _dupenv_s(&val, &val_len, name);

        if (err != 0)
        {
            free(val);
            return com_util_error_report_errno(detail_out, (int)err);
        }
        if (val == NULL)
        {
            if (buf != NULL && buf_size > 0)
            {
                buf[0] = '\0';
            }
            return com_util_error_report_success(detail_out);
        }
        if (exists_out != NULL)
        {
            *exists_out = 1;
        }
        if (buf != NULL)
        {
            if (val_len > buf_size)
            {
                free(val);
                return com_util_error_report_errno(detail_out, ERANGE);
            }
            memcpy(buf, val, val_len);
        }
        free(val);
        return com_util_error_report_success(detail_out);
    }
#endif /* PLATFORM_ */
}

/* 環境変数名として妥当かを判定する。空文字列と '=' を含む名前は POSIX / MSVC の
 * いずれでも不正であるため、プラットフォーム差を出さないよう先に弾く。 */
static int env_name_is_valid(const char *name)
{
    if (name == NULL || name[0] == '\0')
    {
        return 0;
    }
    if (strchr(name, '=') != NULL)
    {
        return 0;
    }
    return 1;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_setenv(const char *name, const char *value, const int overwrite, com_util_error *detail_out)
{
    if (!env_name_is_valid(name) || value == NULL)
    {
        return com_util_error_report_errno(detail_out, EINVAL);
    }

#if defined(PLATFORM_LINUX)
    {
        if (setenv(name, value, overwrite) != 0)
        {
            const int errno_value = errno;

            return com_util_error_report_errno(detail_out, errno_value);
        }
        return com_util_error_report_success(detail_out);
    }
#elif defined(PLATFORM_WINDOWS)
    {
        /* _putenv_s に overwrite 相当の引数はないため、既存の有無を自前で確認する */
        if (overwrite == 0)
        {
            int exists = 0;

            if (com_util_getenv(name, NULL, 0u, &exists, NULL) == COM_UTIL_OK && exists != 0)
            {
                return com_util_error_report_success(detail_out);
            }
        }

        if (_putenv_s(name, value) != 0)
        {
            int errno_value = errno;

            if (errno_value == 0)
            {
                errno_value = EIO;
            }

            return com_util_error_report_errno(detail_out, errno_value);
        }
        return com_util_error_report_success(detail_out);
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_unsetenv(const char *name, com_util_error *detail_out)
{
    if (!env_name_is_valid(name))
    {
        return com_util_error_report_errno(detail_out, EINVAL);
    }

#if defined(PLATFORM_LINUX)
    {
        if (unsetenv(name) != 0)
        {
            const int errno_value = errno;

            return com_util_error_report_errno(detail_out, errno_value);
        }
        return com_util_error_report_success(detail_out);
    }
#elif defined(PLATFORM_WINDOWS)
    {
        /* Windows は値に空文字列を指定した _putenv_s を削除として扱う。
         * see: https://learn.microsoft.com/cpp/c-runtime-library/reference/putenv-s-wputenv-s */
        if (_putenv_s(name, "") != 0)
        {
            int errno_value = errno;

            if (errno_value == 0)
            {
                errno_value = EIO;
            }

            return com_util_error_report_errno(detail_out, errno_value);
        }
        return com_util_error_report_success(detail_out);
    }
#endif /* PLATFORM_ */
}
