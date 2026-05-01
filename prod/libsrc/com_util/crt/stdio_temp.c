#include <com_util/crt/stdio.h>
#include <com_util/crt/path.h>

#include <com_util/crt/crt_internal.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#if defined(PLATFORM_LINUX)
    #include <stdlib.h>
    #include <unistd.h>
#elif defined(PLATFORM_WINDOWS)
    #include <stdlib.h>
    #include <wchar.h>
#endif /* PLATFORM_ */

/* prefix の有効文字数上限 (Windows GetTempFileNameW の制約に準拠) */
#define COM_UTIL_TEMP_PREFIX_MAX 3u

COM_UTIL_EXPORT FILE *COM_UTIL_API com_util_fopen_temp(const char *prefix,
                                                        const char *modes,
                                                        char       *path_out,
                                                        size_t      path_size,
                                                        int        *errno_out)
{
    if (modes == NULL || path_out == NULL || path_size == 0u)
    {
        if (errno_out != NULL)
        {
            *errno_out = EINVAL;
        }
        return NULL;
    }

#if defined(PLATFORM_LINUX)
    {
        const char *tmpdir   = getenv("TMPDIR");
        char        pfx_buf[COM_UTIL_TEMP_PREFIX_MAX + 1u];
        const char *pfx;
        int         fd;
        FILE       *fp;
        int         n;

        if (prefix != NULL)
        {
            if (strlen(prefix) > COM_UTIL_TEMP_PREFIX_MAX)
            {
                memcpy(pfx_buf, prefix, COM_UTIL_TEMP_PREFIX_MAX);
                pfx_buf[COM_UTIL_TEMP_PREFIX_MAX] = '\0';
                pfx = pfx_buf;
            }
            else
            {
                pfx = prefix;
            }
        }
        else
        {
            pfx = "cu_";
        }

        if (tmpdir == NULL || tmpdir[0] == '\0')
        {
            tmpdir = "/tmp";
        }

        n = snprintf(path_out, path_size, "%s" PLATFORM_PATH_SEP "%sXXXXXX", tmpdir, pfx);
        if (n < 0 || (size_t)n >= path_size)
        {
            if (errno_out != NULL)
            {
                *errno_out = ENAMETOOLONG;
            }
            return NULL;
        }

        errno = 0;
        fd    = mkstemp(path_out);
        if (fd == -1)
        {
            if (errno_out != NULL)
            {
                *errno_out = errno;
            }
            return NULL;
        }

        fp = fdopen(fd, modes);
        if (fp == NULL)
        {
            int saved = errno;
            close(fd);
            unlink(path_out);
            if (errno_out != NULL)
            {
                *errno_out = saved;
            }
            return NULL;
        }
        return fp;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wdir[MAX_PATH];
        wchar_t wfile[MAX_PATH];
        wchar_t wprefix[COM_UTIL_TEMP_PREFIX_MAX + 1u];
        wchar_t wmodes[64];
        FILE   *fp = NULL;
        errno_t err;
        size_t  converted;
        DWORD   dwret;
        UINT    uret;

        dwret = GetTempPathW((DWORD)(sizeof(wdir) / sizeof(wdir[0])), wdir);
        if (dwret == 0u || dwret > (DWORD)(sizeof(wdir) / sizeof(wdir[0])))
        {
            if (errno_out != NULL)
            {
                *errno_out = (int)GetLastError();
            }
            return NULL;
        }

        {
            const char *pfx = (prefix != NULL && prefix[0] != '\0') ? prefix : "cu_";
            err = mbstowcs_s(&converted, wprefix, sizeof(wprefix) / sizeof(wprefix[0]), pfx, _TRUNCATE);
            /* STRUNCATE: 4 文字以上の prefix が COM_UTIL_TEMP_PREFIX_MAX に切り詰められた場合。正常扱い。 */
            if (err != 0 && err != STRUNCATE)
            {
                if (errno_out != NULL)
                {
                    *errno_out = EINVAL;
                }
                return NULL;
            }
        }

        uret = GetTempFileNameW(wdir, wprefix, 0u, wfile);
        if (uret == 0u)
        {
            if (errno_out != NULL)
            {
                *errno_out = (int)GetLastError();
            }
            return NULL;
        }

        if (com_util_wpath_to_utf8(path_out, path_size, wfile) < 0)
        {
            DeleteFileW(wfile);
            if (errno_out != NULL)
            {
                *errno_out = ENAMETOOLONG;
            }
            return NULL;
        }

        err = mbstowcs_s(&converted, wmodes, sizeof(wmodes) / sizeof(wmodes[0]), modes, _TRUNCATE);
        if (err != 0)
        {
            DeleteFileW(wfile);
            if (errno_out != NULL)
            {
                *errno_out = EINVAL;
            }
            return NULL;
        }

        err = _wfopen_s(&fp, wfile, wmodes);
        if (err != 0)
        {
            DeleteFileW(wfile);
            if (errno_out != NULL)
            {
                *errno_out = (int)err;
            }
            return NULL;
        }
        return fp;
    }
#endif /* PLATFORM_ */
}
