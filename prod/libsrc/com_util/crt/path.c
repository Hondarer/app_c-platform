/**
 *******************************************************************************
 *  @file           path.c
 *  @brief          UTF-8 パスを操作する API を実装します。
 *
 *  絶対パス化、区切り文字の正規化、パス比較、連結、一時ディレクトリ取得を提供します。
 *
 *******************************************************************************
 */

#include <com_util/crt/path.h>
#include <com_util/crt/wchar_conv.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#if defined(PLATFORM_LINUX)
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
#endif

static int com_util_copy_path_text(char *path_out, const size_t path_size, int *errno_out, const char *text)
{
    size_t len;

    if (path_out == NULL || path_size == 0u || text == NULL)
    {
        if (errno_out != NULL)
        {
            *errno_out = EINVAL;
        }
        return -1;
    }

    len = strlen(text);
    if (len + 1u > path_size)
    {
        path_out[0] = '\0';
        if (errno_out != NULL)
        {
            *errno_out = ENAMETOOLONG;
        }
        return -1;
    }

    memcpy(path_out, text, len + 1u);
    return 0;
}

static int com_util_compare_normalized_paths(const char *lhs, const char *rhs)
{
#if defined(PLATFORM_WINDOWS)
    while (*lhs != '\0' && *rhs != '\0')
    {
        if (tolower((unsigned char)*lhs) != tolower((unsigned char)*rhs))
        {
            return 0;
        }
        lhs++;
        rhs++;
    }
    if (*lhs == '\0' && *rhs == '\0')
    {
        return 1;
    }
    else
    {
        return 0;
    }
#else
    if (strcmp(lhs, rhs) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
#endif
}

#if defined(PLATFORM_LINUX)
static void com_util_normalize_absolute_posix_path(char *path)
{
    size_t read_idx;
    size_t write_idx;
    size_t restore_points[PLATFORM_PATH_MAX];
    size_t restore_count = 0u;

    if (path == NULL || path[0] != PLATFORM_PATH_SEP_CHR)
    {
        return;
    }

    write_idx = 1u;
    read_idx = 1u;

    while (path[read_idx] != '\0')
    {
        size_t seg_start;
        size_t seg_len;

        while (path[read_idx] == PLATFORM_PATH_SEP_CHR)
        {
            read_idx++;
        }

        seg_start = read_idx;
        while (path[read_idx] != '\0' && path[read_idx] != PLATFORM_PATH_SEP_CHR)
        {
            read_idx++;
        }
        seg_len = read_idx - seg_start;

        if (seg_len == 0u)
        {
            break;
        }

        if (seg_len == 1u && path[seg_start] == '.')
        {
            continue;
        }

        if (seg_len == 2u && path[seg_start] == '.' && path[seg_start + 1u] == '.')
        {
            if (restore_count > 0u)
            {
                write_idx = restore_points[--restore_count];
            }
            continue;
        }

        restore_points[restore_count++] = write_idx;
        if (write_idx > 1u)
        {
            path[write_idx++] = PLATFORM_PATH_SEP_CHR;
        }
        memmove(path + write_idx, path + seg_start, seg_len);
        write_idx += seg_len;
    }

    if (write_idx == 0u)
    {
        path[0] = PLATFORM_PATH_SEP_CHR;
        write_idx = 1u;
    }
    path[write_idx] = '\0';
}

static int com_util_build_absolute_posix_path(char *path_out, const size_t path_size, int *errno_out, const char *path)
{
    int written;

    if (path == NULL || path[0] == '\0')
    {
        if (path_out != NULL && path_size > 0u)
        {
            path_out[0] = '\0';
        }
        if (errno_out != NULL)
        {
            *errno_out = EINVAL;
        }
        return -1;
    }

    if (path[0] == PLATFORM_PATH_SEP_CHR)
    {
        return com_util_copy_path_text(path_out, path_size, errno_out, path);
    }

    {
        char cwd[PLATFORM_PATH_MAX];

        if (getcwd(cwd, sizeof(cwd)) == NULL)
        {
            if (path_out != NULL && path_size > 0u)
            {
                path_out[0] = '\0';
            }
            if (errno_out != NULL)
            {
                *errno_out = errno;
            }
            return -1;
        }

        written = snprintf(path_out, path_size, "%s/%s", cwd, path);
        if (written < 0 || (size_t)written >= path_size)
        {
            if (path_out != NULL && path_size > 0u)
            {
                path_out[0] = '\0';
            }
            if (errno_out != NULL)
            {
                *errno_out = ENAMETOOLONG;
            }
            return -1;
        }
    }

    return 0;
}
#endif /* PLATFORM_LINUX */

static int com_util_vpath_concat_n(char *path_out, const size_t path_size, int *errno_out, const size_t part_count,
                                   va_list args)
{
    size_t required_size = 1u;
    size_t offset = 0u;
    size_t idx;
    va_list args_copy;

    if (path_out == NULL || path_size == 0u || part_count == 0u)
    {
        if (errno_out != NULL)
        {
            *errno_out = EINVAL;
        }
        return -1;
    }

    path_out[0] = '\0';

    va_copy(args_copy, args);
    for (idx = 0u; idx < part_count; ++idx)
    {
        const char *part = va_arg(args_copy, const char *);
        size_t part_len;
        if (part == NULL)
        {
            va_end(args_copy);
            if (errno_out != NULL)
            {
                *errno_out = EINVAL;
            }
            return -1;
        }

        part_len = strlen(part);
        if (part_len > path_size - required_size)
        {
            va_end(args_copy);
            if (errno_out != NULL)
            {
                *errno_out = ENAMETOOLONG;
            }
            return -1;
        }
        required_size += part_len;
    }
    va_end(args_copy);

    for (idx = 0u; idx < part_count; ++idx)
    {
        const char *part = va_arg(args, const char *);
        size_t part_len = strlen(part);
        memcpy(path_out + offset, part, part_len);
        offset += part_len;
    }
    path_out[offset] = '\0';

    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

char *com_util_normalize_path_sep(char *path)
{
    char *p;
    for (p = path; *p != '\0'; ++p)
    {
        if (*p == '\\')
        {
            *p = PLATFORM_PATH_SEP_CHR;
        }
    }
    return path;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_path_get_full(char *path_out, const size_t path_size, int *errno_out, const char *path)
{
    if (path_out == NULL || path_size == 0u || path == NULL || path[0] == '\0')
    {
        if (path_out != NULL && path_size > 0u)
        {
            path_out[0] = '\0';
        }
        if (errno_out != NULL)
        {
            *errno_out = EINVAL;
        }
        return -1;
    }

#if defined(PLATFORM_LINUX)
    {
        char candidate[PLATFORM_PATH_MAX];
        char resolved[PLATFORM_PATH_MAX];

        if (com_util_build_absolute_posix_path(candidate, sizeof(candidate), errno_out, path) != 0)
        {
            path_out[0] = '\0';
            return -1;
        }

        com_util_normalize_path_sep(candidate);
        com_util_normalize_absolute_posix_path(candidate);

        if (realpath(candidate, resolved) != NULL)
        {
            return com_util_copy_path_text(path_out, path_size, errno_out, resolved);
        }

        return com_util_copy_path_text(path_out, path_size, errno_out, candidate);
    }
#elif defined(PLATFORM_WINDOWS)
    {
        char normalized_input[PLATFORM_PATH_MAX];
        wchar_t wpath[PLATFORM_PATH_MAX];
        wchar_t wfull[PLATFORM_PATH_MAX];
        DWORD needed;

        if (com_util_copy_path_text(normalized_input, sizeof(normalized_input), errno_out, path) != 0)
        {
            path_out[0] = '\0';
            return -1;
        }
        com_util_normalize_path_sep(normalized_input);

        if (com_util_utf8_to_wpath(wpath, sizeof(wpath) / sizeof(wpath[0]), normalized_input) < 0)
        {
            path_out[0] = '\0';
            if (errno_out != NULL)
            {
                *errno_out = EINVAL;
            }
            return -1;
        }

        needed = GetFullPathNameW(wpath, (DWORD)(sizeof(wfull) / sizeof(wfull[0])), wfull, NULL);
        if (needed == 0u)
        {
            path_out[0] = '\0';
            if (errno_out != NULL)
            {
                *errno_out = (int)GetLastError();
            }
            return -1;
        }
        if (needed >= (DWORD)(sizeof(wfull) / sizeof(wfull[0])))
        {
            path_out[0] = '\0';
            if (errno_out != NULL)
            {
                *errno_out = ENAMETOOLONG;
            }
            return -1;
        }

        if (com_util_wpath_to_utf8(path_out, path_size, wfull) < 0)
        {
            path_out[0] = '\0';
            if (errno_out != NULL)
            {
                *errno_out = ENAMETOOLONG;
            }
            return -1;
        }
    }
    return 0;
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_paths_equal(const char *lhs, const char *rhs, int *errno_out)
{
    char lhs_full[PLATFORM_PATH_MAX];
    char rhs_full[PLATFORM_PATH_MAX];
    int err = 0;

    if (com_util_path_get_full(lhs_full, sizeof(lhs_full), &err, lhs) != 0)
    {
        if (errno_out != NULL)
        {
            *errno_out = err;
        }
        return -1;
    }

    err = 0;
    if (com_util_path_get_full(rhs_full, sizeof(rhs_full), &err, rhs) != 0)
    {
        if (errno_out != NULL)
        {
            *errno_out = err;
        }
        return -1;
    }

    return com_util_compare_normalized_paths(lhs_full, rhs_full);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_get_temp_dir(char *path_out, const size_t path_size, int *errno_out)
{
    if (path_out == NULL || path_size == 0u)
    {
        if (errno_out != NULL)
        {
            *errno_out = EINVAL;
        }
        return -1;
    }

#if defined(PLATFORM_LINUX)
    {
        const char *tmpdir = getenv("TMPDIR");
        size_t len;
        int n;

        if (tmpdir == NULL || tmpdir[0] == '\0')
        {
            tmpdir = "/tmp";
        }

        len = strlen(tmpdir);
        while (len > 1u && tmpdir[len - 1u] == PLATFORM_PATH_SEP_CHR)
        {
            --len;
        }

        n = snprintf(path_out, path_size, "%.*s", (int)len, tmpdir);
        if (n < 0 || (size_t)n >= path_size)
        {
            if (errno_out != NULL)
            {
                *errno_out = ENAMETOOLONG;
            }
            return -1;
        }
        return 0;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wdir[PLATFORM_PATH_MAX];
        DWORD dwret;
        size_t len;

        dwret = GetTempPathW((DWORD)(sizeof(wdir) / sizeof(wdir[0])), wdir);
        if (dwret == 0u || dwret >= (DWORD)(sizeof(wdir) / sizeof(wdir[0])))
        {
            if (errno_out != NULL)
            {
                *errno_out = (int)GetLastError();
            }
            return -1;
        }

        if (com_util_wpath_to_utf8(path_out, path_size, wdir) < 0)
        {
            if (errno_out != NULL)
            {
                *errno_out = ENAMETOOLONG;
            }
            return -1;
        }

        /* GetTempPathW は末尾 '\' を付けて返す。変換後は '/' になるので除去する */
        len = strlen(path_out);
        while (len > 1u && path_out[len - 1u] == PLATFORM_PATH_SEP_CHR)
        {
            path_out[--len] = '\0';
        }
        return 0;
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_path_concat_n(char *path_out, const size_t path_size, int *errno_out, const size_t part_count, ...)
{
    int result;
    va_list args;

    va_start(args, part_count);
    result = com_util_vpath_concat_n(path_out, path_size, errno_out, part_count, args);
    va_end(args);

    return result;
}
