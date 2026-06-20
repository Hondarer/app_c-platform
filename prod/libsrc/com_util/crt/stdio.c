/**
 *******************************************************************************
 *  @file           stdio.c
 *  @brief          stdio 系 CRT 抽象 API の実装です。
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
    #include <stdlib.h>
    #include <wchar.h>
#endif /* PLATFORM_ */

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT FILE *COM_UTIL_API com_util_fopen(const char *path, const char *modes, int *errno_out)
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

        err = _wfopen_s(&fp, wpath, wmodes);
        if (err != 0)
        {
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

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT FILE *COM_UTIL_API com_util_freopen(const char *path, const char *modes, FILE *stream, int *errno_out)
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

        err = _wfreopen_s(&fp, wpath, wmodes, stream);
        if (err != 0)
        {
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

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_remove(const char *path)
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

COM_UTIL_EXPORT int COM_UTIL_API com_util_rename(const char *oldpath, const char *newpath)
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

COM_UTIL_EXPORT int COM_UTIL_API com_util_fclose(FILE *stream)
{
    return fclose(stream);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT size_t COM_UTIL_API com_util_fread(void *ptr, const size_t size, const size_t count, FILE *stream)
{
    return fread(ptr, size, count, stream);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT size_t COM_UTIL_API com_util_fwrite(const void *ptr, const size_t size, const size_t count,
                                                    FILE *stream)
{
    return fwrite(ptr, size, count, stream);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT char *COM_UTIL_API com_util_fgets(char *buf, const int size, FILE *stream)
{
    return fgets(buf, size, stream);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_fputs(const char *str, FILE *stream)
{
    return fputs(str, stream);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_fprintf(FILE *stream, const char *format, ...)
{
    int result;
    va_list args;

    va_start(args, format);
    result = com_util_vfprintf(stream, format, args);
    va_end(args);

    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_vfprintf(FILE *stream, const char *format, va_list args)
{
#if defined(PLATFORM_WINDOWS)
    return vfprintf_s(stream, format, args);
#else
    return vfprintf(stream, format, args);
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_fflush(FILE *stream)
{
    return fflush(stream);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_feof(FILE *stream)
{
    return feof(stream);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_ferror(FILE *stream)
{
    return ferror(stream);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT void COM_UTIL_API com_util_clearerr(FILE *stream)
{
    clearerr(stream);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT void COM_UTIL_API com_util_rewind(FILE *stream)
{
    rewind(stream);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_fseek(FILE *stream, const int64_t offset, const int whence)
{
#if defined(PLATFORM_LINUX)
    return fseeko(stream, (off_t)offset, whence);
#elif defined(PLATFORM_WINDOWS)
    return _fseeki64(stream, (__int64)offset, whence);
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int64_t COM_UTIL_API com_util_ftell(FILE *stream)
{
#if defined(PLATFORM_LINUX)
    return (int64_t)ftello(stream);
#elif defined(PLATFORM_WINDOWS)
    return (int64_t)_ftelli64(stream);
#endif /* PLATFORM_ */
}
