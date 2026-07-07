/**
 *******************************************************************************
 *  @file           unistd.c
 *  @brief          unistd 系の CRT 関数を抽象化する API を実装します。
 *
 *  標準ストリームの端末判定、UTF-8 パスのアクセス確認、
 *  ファイル記述子の操作 (位置移動、クローズ、複製、読み書き) を提供します。
 *
 *******************************************************************************
 */

#include <com_util/crt/unistd.h>
#include <com_util/crt/path.h>

#include <stdio.h>

#include <com_util/crt/wchar_conv.h>

#if defined(PLATFORM_WINDOWS)
    #include <limits.h>
    #include <com_util/base/windows_sdk.h>
#endif /* PLATFORM_WINDOWS */

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_isatty(const com_util_stream_t stream)
{
#if defined(PLATFORM_LINUX)

    int fd;

    if (stream == COM_UTIL_STREAM_STDIN)
    {
        fd = STDIN_FILENO;
    }
    else if (stream == COM_UTIL_STREAM_STDOUT)
    {
        fd = STDOUT_FILENO;
    }
    else if (stream == COM_UTIL_STREAM_STDERR)
    {
        fd = STDERR_FILENO;
    }
    else
    {
        return 0;
    }

    return isatty(fd);

#elif defined(PLATFORM_WINDOWS)

    HANDLE h;
    DWORD mode;
    DWORD std_handle;

    if (stream == COM_UTIL_STREAM_STDIN)
    {
        std_handle = STD_INPUT_HANDLE;
    }
    else if (stream == COM_UTIL_STREAM_STDOUT)
    {
        std_handle = STD_OUTPUT_HANDLE;
    }
    else if (stream == COM_UTIL_STREAM_STDERR)
    {
        std_handle = STD_ERROR_HANDLE;
    }
    else
    {
        return 0;
    }

    h = GetStdHandle(std_handle);
    if (h == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    return (GetFileType(h) == FILE_TYPE_CHAR) && GetConsoleMode(h, &mode);

#else

    (void)stream;
    return 0;

#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int64_t com_util_lseek(const int fd, const int64_t offset, const int whence)
{
    if (fd < 0)
    {
        return -1;
    }

    /* MSVC の _lseeki64 は不正な whence で invalid parameter handler を起動するため、 */
    /* OS の API を呼び出す前に検査する。                                              */
    /* see: https://learn.microsoft.com/cpp/c-runtime-library/reference/lseek-lseeki64 */
    if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    return (int64_t)lseek(fd, (off_t)offset, whence);
#elif defined(PLATFORM_WINDOWS)
    return (int64_t)_lseeki64(fd, offset, whence);
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_close(const int fd)
{
    if (fd < 0)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    return close(fd);
#elif defined(PLATFORM_WINDOWS)
    return _close(fd);
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_dup(const int fd)
{
    if (fd < 0)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    return dup(fd);
#elif defined(PLATFORM_WINDOWS)
    return _dup(fd);
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_dup2(const int oldfd, const int newfd)
{
    if (oldfd < 0 || newfd < 0)
    {
        return -1;
    }

    /* POSIX の dup2 は成功時に newfd を、Windows の _dup2 は 0 を返す。 */
    /* 本関数は成功時 0 に正規化する。                                   */
#if defined(PLATFORM_LINUX)
    if (dup2(oldfd, newfd) == -1)
    {
        return -1;
    }
    return 0;
#elif defined(PLATFORM_WINDOWS)
    if (_dup2(oldfd, newfd) != 0)
    {
        return -1;
    }
    return 0;
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int64_t com_util_read(const int fd, void *buf, const size_t count)
{
    if (fd < 0 || buf == NULL)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    return (int64_t)read(fd, buf, count);
#elif defined(PLATFORM_WINDOWS)
    {
        unsigned int request;

        /* _read の引数は unsigned int のため、INT_MAX に切り詰める (部分読み取り)。 */
        if (count > (size_t)INT_MAX)
        {
            request = (unsigned int)INT_MAX;
        }
        else
        {
            request = (unsigned int)count;
        }

        return (int64_t)_read(fd, buf, request);
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int64_t com_util_write(const int fd, const void *buf, const size_t count)
{
    if (fd < 0 || buf == NULL)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    return (int64_t)write(fd, buf, count);
#elif defined(PLATFORM_WINDOWS)
    {
        unsigned int request;

        /* _write の引数は unsigned int のため、INT_MAX に切り詰める (部分書き込み)。 */
        if (count > (size_t)INT_MAX)
        {
            request = (unsigned int)INT_MAX;
        }
        else
        {
            request = (unsigned int)count;
        }

        return (int64_t)_write(fd, buf, request);
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_access(const char *path, const int mode)
{
    if (path == NULL)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    return access(path, mode);
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wpath[PLATFORM_PATH_MAX];

        if (com_util_utf8_to_wpath(wpath, sizeof(wpath) / sizeof(wpath[0]), path) < 0)
        {
            return -1;
        }

        return _waccess(wpath, mode);
    }
#endif /* PLATFORM_ */
}
