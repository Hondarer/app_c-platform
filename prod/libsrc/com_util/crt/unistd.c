/**
 *******************************************************************************
 *  @file           unistd.c
 *  @brief          unistd 系の CRT 関数を抽象化する API を実装します。
 *
 *  標準ストリームの端末判定と UTF-8 パスのアクセス確認を提供します。
 *
 *******************************************************************************
 */

#include <com_util/crt/unistd.h>
#include <com_util/crt/path.h>

#include <com_util/crt/wchar_conv.h>

#if defined(PLATFORM_WINDOWS)
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
