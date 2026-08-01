/**
 *******************************************************************************
 *  @file           fcntl.c
 *  @brief          UTF-8 パスに対応したファイル記述子オープン API を実装します。
 *
 *  OS ごとの文字コード変換と open() 呼び出しの差異を吸収します。
 *
 *******************************************************************************
 */

#include <com_util/crt/fcntl.h>
#include <com_util/crt/path.h>

#include <com_util/crt/wchar_conv.h>
#include <com_util/base/error_internal.h>

#include <errno.h>

#if defined(PLATFORM_LINUX)
    #include <unistd.h>
#elif defined(PLATFORM_WINDOWS)
    #include <share.h>
#endif /* PLATFORM_ */

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_open(const char *path, const int flags, const int mode, com_util_error *detail_out)
{
    int fd;

    if (path == NULL)
    {
        (void)com_util_error_report_errno(detail_out, EINVAL);
        return -1;
    }

#if defined(PLATFORM_LINUX)
    errno = 0;
    fd = open(path, flags, mode);
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wpath[PLATFORM_PATH_MAX];
        errno_t err;

        fd = -1;

        if (com_util_utf8_to_wpath(wpath, sizeof(wpath) / sizeof(wpath[0]), path) < 0)
        {
            (void)com_util_error_report_errno(detail_out, ENAMETOOLONG);
            return -1;
        }

        err = _wsopen_s(&fd, wpath, flags, _SH_DENYNO, mode);
        if (err != 0)
        {
            (void)com_util_error_report_errno(detail_out, (int)err);
            return -1;
        }
    }
#endif /* PLATFORM_ */

    if (fd < 0)
    {
        const int errno_value = errno;

        (void)com_util_error_report_errno(detail_out, errno_value);
    }
    else
    {
        (void)com_util_error_report_success(detail_out);
    }

    return fd;
}
