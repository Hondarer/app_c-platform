/**
 *******************************************************************************
 *  @file           stdio_temp.c
 *  @brief          一時ファイルを生成する API を実装します。
 *
 *  競合しない一時ファイルを生成し、ファイル ストリームと絶対パスを返します。
 *
 *******************************************************************************
 */

#define _GNU_SOURCE

#include <com_util/crt/stdio.h>
#include <com_util/crt/path.h>

#include <com_util/crt/wchar_conv.h>
#include <com_util/crt/stdlib.h>
#include <com_util/base/error_internal.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#if defined(PLATFORM_LINUX)
    #include <fcntl.h>
    #include <stdlib.h>
    #include <unistd.h>
#elif defined(PLATFORM_WINDOWS)
    #include <share.h>
    #include <stdlib.h>
    #include <wchar.h>
#endif /* PLATFORM_ */

/* prefix の有効文字数上限 (Windows GetTempFileNameW の制約に準拠) */
#define COM_UTIL_TEMP_PREFIX_MAX 3u

/* Doxygen コメントは、ヘッダーに記載 */

FILE *com_util_fopen_temp(const char *prefix, const char *modes, char *path_out, const size_t path_size,
                          com_util_error *detail_out)
{
    if (modes == NULL || path_out == NULL || path_size == 0u)
    {
        (void)com_util_error_report_errno(detail_out, EINVAL);
        return NULL;
    }

#if defined(PLATFORM_LINUX)
    {
        char tmpdir_buf[PLATFORM_PATH_MAX];
        const char *tmpdir;
        char pfx_buf[COM_UTIL_TEMP_PREFIX_MAX + 1u];
        const char *pfx;
        int fd;
        FILE *fp;
        int n;

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

        /* TMPDIR が本バッファーに収まらない場合、切り詰めた値では誤ったディレクトリに
           一時ファイルを作成するため、失敗として扱う */
        if (com_util_getenv("TMPDIR", tmpdir_buf, sizeof(tmpdir_buf), NULL) == ERANGE)
        {
            (void)com_util_error_report_errno(detail_out, ENAMETOOLONG);
            return NULL;
        }
        if (tmpdir_buf[0] == '\0')
        {
            tmpdir = "/tmp";
        }
        else
        {
            tmpdir = tmpdir_buf;
        }

        n = snprintf(path_out, path_size, "%s" PLATFORM_PATH_SEP "%sXXXXXX", tmpdir, pfx);
        if (n < 0 || (size_t)n >= path_size)
        {
            (void)com_util_error_report_errno(detail_out, ENAMETOOLONG);
            return NULL;
        }

        errno = 0;
        fd = mkostemp(path_out, O_CLOEXEC);
        if (fd == -1)
        {
            const int errno_value = errno;

            (void)com_util_error_report_errno(detail_out, errno_value);
            return NULL;
        }

        fp = fdopen(fd, modes);
        if (fp == NULL)
        {
            int saved = errno;
            close(fd);
            unlink(path_out);
            (void)com_util_error_report_errno(detail_out, saved);
            return NULL;
        }
        (void)com_util_error_report_success(detail_out);
        return fp;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wdir[MAX_PATH];
        wchar_t wfile[MAX_PATH];
        wchar_t wprefix[COM_UTIL_TEMP_PREFIX_MAX + 1u];
        wchar_t wmodes[64];
        FILE *fp = NULL;
        errno_t err;
        size_t converted;
        DWORD dwret;
        UINT uret;

        /* 以降の GetTempFileNameW / DeleteFileW へワイドのまま渡すため、
           *U ラッパーを経由しない (UTF-8 への往復変換が増えるだけになる) */
        dwret = GetTempPathW((DWORD)(sizeof(wdir) / sizeof(wdir[0])), wdir);
        if (dwret == 0u)
        {
            const DWORD error_code = GetLastError();

            (void)com_util_error_report_windows_error(detail_out, error_code);
            return NULL;
        }
        if (dwret > (DWORD)(sizeof(wdir) / sizeof(wdir[0])))
        {
            (void)com_util_error_report_errno(detail_out, ENAMETOOLONG);
            return NULL;
        }

        {
            const char *pfx;
            if (prefix != NULL && prefix[0] != '\0')
            {
                pfx = prefix;
            }
            else
            {
                pfx = "cu_";
            }
            err = mbstowcs_s(&converted, wprefix, sizeof(wprefix) / sizeof(wprefix[0]), pfx, _TRUNCATE);
            /* STRUNCATE: 4 文字以上の prefix が COM_UTIL_TEMP_PREFIX_MAX に切り詰められた場合。正常扱い。 */
            if (err != 0 && err != STRUNCATE)
            {
                (void)com_util_error_report_errno(detail_out, EINVAL);
                return NULL;
            }
        }

        uret = GetTempFileNameW(wdir, wprefix, 0u, wfile);
        if (uret == 0u)
        {
            const DWORD error_code = GetLastError();

            (void)com_util_error_report_windows_error(detail_out, error_code);
            return NULL;
        }

        if (com_util_wpath_to_utf8(path_out, path_size, wfile) < 0)
        {
            DeleteFileW(wfile);
            (void)com_util_error_report_errno(detail_out, ENAMETOOLONG);
            return NULL;
        }

        err = mbstowcs_s(&converted, wmodes, sizeof(wmodes) / sizeof(wmodes[0]), modes, _TRUNCATE);
        if (err != 0)
        {
            DeleteFileW(wfile);
            (void)com_util_error_report_errno(detail_out, EINVAL);
            return NULL;
        }

        /* Linux 側 (fdopen 経由) と挙動をそろえるため _wfsopen + _SH_DENYNO を採用する。
         * see: https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/fsopen-wfsopen */
        errno = 0;
        fp = _wfsopen(wfile, wmodes, _SH_DENYNO);
        if (fp == NULL)
        {
            int saved = errno;
            DeleteFileW(wfile);
            (void)com_util_error_report_errno(detail_out, saved);
            return NULL;
        }
        (void)com_util_error_report_success(detail_out);
        return fp;
    }
#endif /* PLATFORM_ */
}
