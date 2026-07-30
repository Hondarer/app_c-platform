/**
 *******************************************************************************
 *  @file           stdio.c
 *  @brief          stdio 系の C 標準入出力関数を抽象化する API を実装します。
 *
 *  UTF-8 パスと 64 bit ファイル位置に対応した標準 I/O ラッパーを提供します。
 *
 *******************************************************************************
 */

#include <com_util/crt/stdio.h>
#include <com_util/crt/path.h>

#include <com_util/crt/wchar_conv.h>

#include <errno.h>
#include <string.h>

#if defined(PLATFORM_LINUX)
    #include <sys/types.h>
#elif defined(PLATFORM_WINDOWS)
    #include <io.h>
    #include <share.h>
    #include <stdlib.h>
    #include <wchar.h>
#endif /* PLATFORM_ */

/* Doxygen コメントは、ヘッダーに記載 */

FILE *com_util_fopen(const char *path, const char *modes, int *errno_out)
{
    if (path == NULL || modes == NULL)
    {
        if (errno_out != NULL)
        {
            *errno_out = EINVAL;
        }
        return NULL;
    }

#if defined(PLATFORM_LINUX)
    {
        FILE *fp;
        errno = 0;
        fp = fopen(path, modes);
        if (fp == NULL && errno_out != NULL)
        {
            *errno_out = errno;
        }
        return fp;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wpath[PLATFORM_PATH_MAX];
        wchar_t wmodes[64];
        FILE *fp = NULL;
        errno_t err;
        size_t converted;

        if (com_util_utf8_to_wpath(wpath, sizeof(wpath) / sizeof(wpath[0]), path) < 0)
        {
            if (errno_out != NULL)
            {
                *errno_out = ENAMETOOLONG;
            }
            return NULL;
        }

        err = mbstowcs_s(&converted, wmodes, sizeof(wmodes) / sizeof(wmodes[0]), modes, _TRUNCATE);
        if (err != 0)
        {
            if (errno_out != NULL)
            {
                *errno_out = EINVAL;
            }
            return NULL;
        }

        /* Linux の fopen は強制ロックを持たず常に共有可。_wfopen_s は排他オープンとなるため、
         * 挙動をそろえるために _wfsopen + _SH_DENYNO を採用する。
         * see: https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/fsopen-wfsopen */
        errno = 0;
        fp = _wfsopen(wpath, wmodes, _SH_DENYNO);
        if (fp == NULL)
        {
            if (errno_out != NULL)
            {
                *errno_out = errno;
            }
            return NULL;
        }

        return fp;
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

FILE *com_util_freopen(const char *path, const char *modes, FILE *stream, int *errno_out)
{
    if (path == NULL || modes == NULL || stream == NULL)
    {
        if (errno_out != NULL)
        {
            *errno_out = EINVAL;
        }
        return NULL;
    }

#if defined(PLATFORM_LINUX)
    {
        FILE *fp;
        errno = 0;
        fp = freopen(path, modes, stream);
        if (fp == NULL && errno_out != NULL)
        {
            *errno_out = errno;
        }
        return fp;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wpath[PLATFORM_PATH_MAX];
        wchar_t wmodes[64];
        FILE *new_fp = NULL;
        errno_t err;
        size_t converted;
        int new_fd;
        int old_fd;

        if (com_util_utf8_to_wpath(wpath, sizeof(wpath) / sizeof(wpath[0]), path) < 0)
        {
            if (errno_out != NULL)
            {
                *errno_out = ENAMETOOLONG;
            }
            return NULL;
        }

        err = mbstowcs_s(&converted, wmodes, sizeof(wmodes) / sizeof(wmodes[0]), modes, _TRUNCATE);
        if (err != 0)
        {
            if (errno_out != NULL)
            {
                *errno_out = EINVAL;
            }
            return NULL;
        }

        /* Windows CRT には _fsopen 相当の freopen 版が無いため、共有モードで開いた
         * 新規 FILE* の fd を _dup2 で既存 stream に複製して FILE* を維持する。
         * これにより Linux の freopen と同じく共有可能なオープン挙動になる。
         * see: https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/fsopen-wfsopen */
        errno = 0;
        new_fp = _wfsopen(wpath, wmodes, _SH_DENYNO);
        if (new_fp == NULL)
        {
            if (errno_out != NULL)
            {
                *errno_out = errno;
            }
            return NULL;
        }

        (void)fflush(stream);
        new_fd = _fileno(new_fp);
        old_fd = _fileno(stream);
        if (_dup2(new_fd, old_fd) != 0)
        {
            int saved = errno;
            (void)fclose(new_fp);
            if (errno_out != NULL)
            {
                *errno_out = saved;
            }
            return NULL;
        }
        (void)fclose(new_fp); /* new_fd を解放。stream は old_fd を介して新ファイルを保持 */
        clearerr(stream);
        return stream;
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_remove(const char *path)
{
    if (path == NULL)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    return remove(path);
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wpath[PLATFORM_PATH_MAX];

        if (com_util_utf8_to_wpath(wpath, sizeof(wpath) / sizeof(wpath[0]), path) < 0)
        {
            return -1;
        }

        return _wremove(wpath);
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_rename(const char *oldpath, const char *newpath)
{
    if (oldpath == NULL || newpath == NULL)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    return rename(oldpath, newpath);
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t woldpath[PLATFORM_PATH_MAX];
        wchar_t wnewpath[PLATFORM_PATH_MAX];

        if (com_util_utf8_to_wpath(woldpath, sizeof(woldpath) / sizeof(woldpath[0]), oldpath) < 0)
        {
            return -1;
        }

        if (com_util_utf8_to_wpath(wnewpath, sizeof(wnewpath) / sizeof(wnewpath[0]), newpath) < 0)
        {
            return -1;
        }

        if (!MoveFileExW(woldpath, wnewpath, MOVEFILE_REPLACE_EXISTING))
        {
            return -1;
        }
        return 0;
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_fprintf(FILE *stream, const char *format, ...)
{
    int result;
    va_list args;

    va_start(args, format);
    result = com_util_vfprintf(stream, format, args);
    va_end(args);

    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_vfprintf(FILE *stream, const char *format, va_list args)
{
#if defined(PLATFORM_WINDOWS)
    return vfprintf_s(stream, format, args);
#else
    return vfprintf(stream, format, args);
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_fseek(FILE *stream, const int64_t offset, const int whence)
{
#if defined(PLATFORM_LINUX)
    return fseeko(stream, (off_t)offset, whence);
#elif defined(PLATFORM_WINDOWS)
    return _fseeki64(stream, (__int64)offset, whence);
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int64_t com_util_ftell(FILE *stream)
{
#if defined(PLATFORM_LINUX)
    return (int64_t)ftello(stream);
#elif defined(PLATFORM_WINDOWS)
    return (int64_t)_ftelli64(stream);
#endif /* PLATFORM_ */
}
