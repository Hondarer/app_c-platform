#include <com_util/crt/path.h>
#include <com_util/crt/crt_internal.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#if defined(PLATFORM_LINUX)
    #include <stdlib.h>
    #include <stdio.h>
#endif

static int com_util_vpath_concat_n(char *path_out,
                                   size_t path_size,
                                   int *errno_out,
                                   size_t part_count,
                                   va_list args)
{
    size_t required_size = 1u;
    size_t offset = 0u;
    size_t idx;
    va_list args_copy;

    if (path_out == NULL || path_size == 0u || part_count == 0u)
    {
        if (errno_out != NULL) { *errno_out = EINVAL; }
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
            if (errno_out != NULL) { *errno_out = EINVAL; }
            return -1;
        }

        part_len = strlen(part);
        if (part_len > path_size - required_size)
        {
            va_end(args_copy);
            if (errno_out != NULL) { *errno_out = ENAMETOOLONG; }
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

COM_UTIL_EXPORT char *COM_UTIL_API com_util_normalize_path_sep(char *path)
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

COM_UTIL_EXPORT int COM_UTIL_API com_util_get_temp_dir(char   *path_out,
                                                        size_t  path_size,
                                                        int    *errno_out)
{
    if (path_out == NULL || path_size == 0u)
    {
        if (errno_out != NULL) { *errno_out = EINVAL; }
        return -1;
    }

#if defined(PLATFORM_LINUX)
    {
        const char *tmpdir = getenv("TMPDIR");
        size_t      len;
        int         n;

        if (tmpdir == NULL || tmpdir[0] == '\0')
        {
            tmpdir = "/tmp";
        }

        len = strlen(tmpdir);
        while (len > 1u && tmpdir[len - 1u] == PLATFORM_PATH_SEP_CHR) { --len; }

        n = snprintf(path_out, path_size, "%.*s", (int)len, tmpdir);
        if (n < 0 || (size_t)n >= path_size)
        {
            if (errno_out != NULL) { *errno_out = ENAMETOOLONG; }
            return -1;
        }
        return 0;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wdir[PLATFORM_PATH_MAX];
        DWORD   dwret;
        size_t  len;

        dwret = GetTempPathW((DWORD)(sizeof(wdir) / sizeof(wdir[0])), wdir);
        if (dwret == 0u || dwret >= (DWORD)(sizeof(wdir) / sizeof(wdir[0])))
        {
            if (errno_out != NULL) { *errno_out = (int)GetLastError(); }
            return -1;
        }

        if (com_util_wpath_to_utf8(path_out, path_size, wdir) < 0)
        {
            if (errno_out != NULL) { *errno_out = ENAMETOOLONG; }
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

COM_UTIL_EXPORT int COM_UTIL_API com_util_path_concat_n(char   *path_out,
                                                         size_t  path_size,
                                                         int    *errno_out,
                                                         size_t  part_count,
                                                         ...)
{
    int result;
    va_list args;

    va_start(args, part_count);
    result = com_util_vpath_concat_n(path_out, path_size, errno_out, part_count, args);
    va_end(args);

    return result;
}
