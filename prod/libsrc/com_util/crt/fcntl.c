/**
 *******************************************************************************
 *  @file           fcntl.c
 *  @brief          UTF-8 パス対応のファイル記述子オープン API の実装です。
 *
 *  OS ごとの文字コード変換と open() 呼び出しの差異を吸収します。
 *
 *******************************************************************************
 */

#include <com_util/crt/fcntl.h>
#include <com_util/crt/path.h>

#include <com_util/crt/wchar_conv.h>

#if defined(PLATFORM_LINUX)
    #include <unistd.h>
#elif defined(PLATFORM_WINDOWS)
    #include <share.h>
#endif /* PLATFORM_ */

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_open(const char *path, const int flags, const int mode)
{
    if (path == NULL)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    return open(path, flags, mode);
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wpath[PLATFORM_PATH_MAX];
        int fd = -1;
        errno_t err;

        if (com_util_utf8_to_wpath(wpath, sizeof(wpath) / sizeof(wpath[0]), path) < 0)
        {
            return -1;
        }

        err = _wsopen_s(&fd, wpath, flags, _SH_DENYNO, mode);
        if (err != 0)
        {
            return -1;
        }

        return fd;
    }
#endif /* PLATFORM_ */
}
