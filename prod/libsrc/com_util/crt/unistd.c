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
#include <com_util/base/error_internal.h>

#include <errno.h>

#if defined(PLATFORM_WINDOWS)
    #include <limits.h>
    #include <com_util/base/windows_sdk.h>
#endif /* PLATFORM_WINDOWS */

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_isatty(const com_util_stream stream)
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

int64_t com_util_lseek(const int fd, const int64_t offset, const int whence, com_util_error *detail_out)
{
    int64_t result;

    if (fd < 0)
    {
        (void)com_util_error_report_errno(detail_out, EBADF);
        return -1;
    }

    /* MSVC の _lseeki64 は不正な whence で invalid parameter handler を起動するため、 */
    /* OS の API を呼び出す前に検査する。                                              */
    /* see: https://learn.microsoft.com/cpp/c-runtime-library/reference/lseek-lseeki64 */
    if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
    {
        (void)com_util_error_report_errno(detail_out, EINVAL);
        return -1;
    }

    errno = 0;
#if defined(PLATFORM_LINUX)
    result = (int64_t)lseek(fd, (off_t)offset, whence);
#elif defined(PLATFORM_WINDOWS)
    result = (int64_t)_lseeki64(fd, offset, whence);
#endif /* PLATFORM_ */

    if (result < 0)
    {
        (void)com_util_error_report_errno(detail_out, errno);
    }
    else
    {
        (void)com_util_error_report_success(detail_out);
    }
    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_close(const int fd, com_util_error *detail_out)
{
    int result;

    if (fd < 0)
    {
        (void)com_util_error_report_errno(detail_out, EBADF);
        return -1;
    }

    errno = 0;
#if defined(PLATFORM_LINUX)
    /* EINTR で復帰した場合も再試行しない。Linux では中断された時点で記述子が
       解放済みであり、呼び直すと別スレッドが再利用した記述子を閉じうる。
       see: https://man7.org/linux/man-pages/man2/close.2.html */
    result = close(fd);
#elif defined(PLATFORM_WINDOWS)
    result = _close(fd);
#endif /* PLATFORM_ */

    if (result != 0)
    {
        (void)com_util_error_report_errno(detail_out, errno);
    }
    else
    {
        (void)com_util_error_report_success(detail_out);
    }
    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_dup(const int fd, com_util_error *detail_out)
{
    int result;

    if (fd < 0)
    {
        (void)com_util_error_report_errno(detail_out, EBADF);
        return -1;
    }

    errno = 0;
#if defined(PLATFORM_LINUX)
    result = dup(fd);
#elif defined(PLATFORM_WINDOWS)
    result = _dup(fd);
#endif /* PLATFORM_ */

    if (result < 0)
    {
        (void)com_util_error_report_errno(detail_out, errno);
    }
    else
    {
        (void)com_util_error_report_success(detail_out);
    }
    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_dup2(const int oldfd, const int newfd, com_util_error *detail_out)
{
    if (oldfd < 0 || newfd < 0)
    {
        (void)com_util_error_report_errno(detail_out, EBADF);
        return -1;
    }

    /* POSIX の dup2 は成功時に newfd を、Windows の _dup2 は 0 を返す。 */
    /* 本関数は成功時 0 に正規化する。                                   */
#if defined(PLATFORM_LINUX)
    if (dup2(oldfd, newfd) == -1)
    {
        (void)com_util_error_report_errno(detail_out, errno);
        return -1;
    }
#elif defined(PLATFORM_WINDOWS)
    if (_dup2(oldfd, newfd) != 0)
    {
        (void)com_util_error_report_errno(detail_out, errno);
        return -1;
    }
#endif /* PLATFORM_ */
    (void)com_util_error_report_success(detail_out);
    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

int64_t com_util_read(const int fd, void *buf, const size_t count, com_util_error *detail_out)
{
    int64_t result;

    if (fd < 0 || buf == NULL)
    {
        (void)com_util_error_report_errno(detail_out, EINVAL);
        return -1;
    }

    errno = 0;
#if defined(PLATFORM_LINUX)
    /* シグナルによる中断を吸収する。EINTR を利用者へ観測させない規範に従う。 */
    do
    {
        result = (int64_t)read(fd, buf, count);
    } while ((result < 0) && (errno == EINTR));
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

        result = (int64_t)_read(fd, buf, request);
    }
#endif /* PLATFORM_ */

    if (result < 0)
    {
        (void)com_util_error_report_errno(detail_out, errno);
    }
    else
    {
        (void)com_util_error_report_success(detail_out);
    }
    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

int64_t com_util_write(const int fd, const void *buf, const size_t count, com_util_error *detail_out)
{
    int64_t result;

    if (fd < 0 || buf == NULL)
    {
        (void)com_util_error_report_errno(detail_out, EINVAL);
        return -1;
    }

    errno = 0;
#if defined(PLATFORM_LINUX)
    /* シグナルによる中断を吸収する。EINTR を利用者へ観測させない規範に従う。 */
    do
    {
        result = (int64_t)write(fd, buf, count);
    } while ((result < 0) && (errno == EINTR));
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

        result = (int64_t)_write(fd, buf, request);
    }
#endif /* PLATFORM_ */

    if (result < 0)
    {
        (void)com_util_error_report_errno(detail_out, errno);
    }
    else
    {
        (void)com_util_error_report_success(detail_out);
    }
    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_access(const char *path, const int mode, com_util_error *detail_out)
{
    int result;

    if (path == NULL)
    {
        (void)com_util_error_report_errno(detail_out, EINVAL);
        return -1;
    }

    errno = 0;
#if defined(PLATFORM_LINUX)
    result = access(path, mode);
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wpath[PLATFORM_PATH_MAX];

        if (com_util_utf8_to_wpath(wpath, sizeof(wpath) / sizeof(wpath[0]), path) < 0)
        {
            (void)com_util_error_report_errno(detail_out, ENAMETOOLONG);
            return -1;
        }

        result = _waccess(wpath, mode);
    }
#endif /* PLATFORM_ */

    if (result != 0)
    {
        (void)com_util_error_report_errno(detail_out, errno);
    }
    else
    {
        (void)com_util_error_report_success(detail_out);
    }
    return result;
}
