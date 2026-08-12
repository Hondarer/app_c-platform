/**
 *******************************************************************************
 *  @file           stdlib.c
 *  @brief          stdlib 系の CRT 関数を抽象化する API を実装します。
 *
 *  環境変数の有無とバッファー不足を共通の戻り値へ変換します。\n
 *  文字列から数値への変換は、変換位置と `errno` の検査を関数側へ内包し、共通結果コードへ変換します。\n
 *  メモリ確保は、長さ 0 と要素数の乗算オーバーフローの検査を関数側へ内包します。
 *
 *******************************************************************************
 */

#include <com_util/crt/stdlib.h>
#include <com_util/base/error_internal.h>
#include <com_util/base/platform.h>

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
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

/**
 *  @brief          strtoll 系へ渡す基数として妥当かを判定します。
 *  @param[in]      base 判定する基数。
 *  @return         妥当な場合は 1、そうでない場合は 0 を返します。
 *
 *  0 (接頭辞による自動判別) と 2 から 36 を妥当とします。
 *  これ以外の値に対する strtoll 系の動作は未定義であるため、変換前に弾きます。
 */
static int parse_base_is_valid(const int base)
{
    if (base == 0)
    {
        return 1;
    }
    if ((base >= 2) && (base <= 36))
    {
        return 1;
    }

    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_parse_int64(int64_t *value_out, const char *text, const int base)
{
    char *end;
    long long parsed;

    if (value_out == NULL || text == NULL || parse_base_is_valid(base) == 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    end = NULL;
    errno = 0;
    parsed = strtoll(text, &end, base);

    /* end == text は 1 文字も変換できなかったことを表す。空文字列はこの条件だけで検出される。 */
    if (end == text || *end != '\0')
    {
        return COM_UTIL_ERR_INVALID_INTEGER;
    }
    if (errno == ERANGE)
    {
        return COM_UTIL_ERR_OUT_OF_RANGE;
    }

#if LLONG_MAX > INT64_MAX
    /* long long が 64 bit を超える処理系では、int64_t の範囲を別途検査する。 */
    if ((parsed < (long long)INT64_MIN) || (parsed > (long long)INT64_MAX))
    {
        return COM_UTIL_ERR_OUT_OF_RANGE;
    }
#endif /* LLONG_MAX > INT64_MAX */

    *value_out = (int64_t)parsed;

    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_parse_uint64(uint64_t *value_out, const char *text, const int base)
{
    const char *cursor;
    char *end;
    unsigned long long parsed;

    if (value_out == NULL || text == NULL || parse_base_is_valid(base) == 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    /* strtoull は '-' 付きの入力を符号なしの折り返し値として受け付けるため、変換前に拒否する。
     * see: https://en.cppreference.com/w/c/string/byte/strtoul */
    cursor = text;
    while (isspace((unsigned char)*cursor) != 0)
    {
        cursor++;
    }
    if (*cursor == '-')
    {
        return COM_UTIL_ERR_OUT_OF_RANGE;
    }

    end = NULL;
    errno = 0;
    parsed = strtoull(text, &end, base);

    if (end == text || *end != '\0')
    {
        return COM_UTIL_ERR_INVALID_INTEGER;
    }
    if (errno == ERANGE)
    {
        return COM_UTIL_ERR_OUT_OF_RANGE;
    }

#if ULLONG_MAX > UINT64_MAX
    /* unsigned long long が 64 bit を超える処理系では、uint64_t の範囲を別途検査する。 */
    if (parsed > (unsigned long long)UINT64_MAX)
    {
        return COM_UTIL_ERR_OUT_OF_RANGE;
    }
#endif /* ULLONG_MAX > UINT64_MAX */

    *value_out = (uint64_t)parsed;

    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_parse_int(int *value_out, const char *text, const int base)
{
    int64_t parsed;
    int ret;

    if (value_out == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    parsed = 0;
    ret = com_util_parse_int64(&parsed, text, base);
    if (ret != COM_UTIL_OK)
    {
        return ret;
    }
    if ((parsed < (int64_t)INT_MIN) || (parsed > (int64_t)INT_MAX))
    {
        return COM_UTIL_ERR_OUT_OF_RANGE;
    }

    *value_out = (int)parsed;

    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_parse_double(double *value_out, const char *text)
{
    char *end;
    double parsed;

    if (value_out == NULL || text == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    end = NULL;
    errno = 0;
    parsed = strtod(text, &end);

    if (end == text || *end != '\0')
    {
        return COM_UTIL_ERR_INVALID_INTEGER;
    }

    /* strtod はオーバーフローとアンダーフローの双方で ERANGE を設定する。
     * see: https://en.cppreference.com/w/c/string/byte/strtof */
    if (errno == ERANGE)
    {
        return COM_UTIL_ERR_OUT_OF_RANGE;
    }

    *value_out = parsed;

    return COM_UTIL_OK;
}

/* NOTE: 以降の確保・解放関数は、上位規範「動的メモリの確保と解放」および
 *       com_util 規範「メモリ確保の代替」が定める代替の実体である。
 *       本ファイル内の malloc / calloc / realloc / free の直接呼び出しは、
 *       ラッパーが元関数へ委譲するための意図的な例外であり、置換対象外とする。 */

/**
 *  @brief          要素数と要素サイズの乗算が size_t を回り込むかを検査します。
 *  @param[in]      count  要素数。
 *  @param[in]      size   要素 1 個あたりのバイト数。
 *  @return         確保してはならない場合は 1、確保してよい場合は 0 を返します。
 *
 *  いずれかが 0 の場合も、長さ 0 の確保を行わないため 1 を返します。
 */
static int alloc_size_rejects(const size_t count, const size_t size)
{
    if ((count == 0U) || (size == 0U))
    {
        return 1;
    }

    if (count > (SIZE_MAX / size))
    {
        return 1;
    }

    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

void *com_util_malloc(const size_t size)
{
    if (size == 0U)
    {
        return NULL;
    }

    return malloc(size); /* ラッパーの実体。置換対象外 */
}

/* Doxygen コメントは、ヘッダーに記載 */

void *com_util_malloc_zerofill(const size_t size)
{
    void *ptr;

    if (size == 0U)
    {
        return NULL;
    }

    ptr = calloc(1U, size); /* ラッパーの実体。置換対象外 */

    return ptr;
}

/* Doxygen コメントは、ヘッダーに記載 */

void *com_util_calloc(const size_t count, const size_t size)
{
    if (alloc_size_rejects(count, size) != 0)
    {
        return NULL;
    }

    return calloc(count, size); /* ラッパーの実体。置換対象外 */
}

/* Doxygen コメントは、ヘッダーに記載 */

void *com_util_realloc(void *ptr, const size_t count, const size_t size)
{
    if (alloc_size_rejects(count, size) != 0)
    {
        return NULL;
    }

    return realloc(ptr, count * size); /* ラッパーの実体。置換対象外 */
}

/* Doxygen コメントは、ヘッダーに記載 */

void *com_util_realloc_zerofill(void *ptr, const size_t old_count, const size_t count, const size_t size)
{
    void *new_ptr;

    if (alloc_size_rejects(count, size) != 0)
    {
        return NULL;
    }

    new_ptr = realloc(ptr, count * size); /* ラッパーの実体。置換対象外 */
    if (new_ptr == NULL)
    {
        return NULL;
    }

    if (old_count < count)
    {
        /* 拡張した範囲のみをゼロ初期化する。
         * old_count は count 未満であり、count * size は上で回り込みを検査済みのため、
         * old_count * size と (count - old_count) * size はいずれも回り込まない。 */
        memset((char *)new_ptr + (old_count * size), 0, (count - old_count) * size);
    }

    return new_ptr;
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_free(void *ptr)
{
    free(ptr); /* ラッパーの実体。置換対象外。free(NULL) は無害 */
}
