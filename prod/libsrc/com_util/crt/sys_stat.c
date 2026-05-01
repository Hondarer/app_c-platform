#include <com_util/crt/sys/stat.h>
#include <com_util/crt/path.h>

#include <com_util/crt/crt_internal.h>

#include <stddef.h>

#if defined(PLATFORM_LINUX)
    #include <sys/stat.h>
#elif defined(PLATFORM_WINDOWS)
    #include <direct.h>
#endif /* PLATFORM_ */

COM_UTIL_EXPORT int COM_UTIL_API com_util_stat(com_util_file_stat_t *buf,
                                                const char       *path)
{
    if (buf == NULL || path == NULL)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    return stat(path, buf);
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wpath[PLATFORM_PATH_MAX];

        if (com_util_utf8_to_wpath(wpath, sizeof(wpath) / sizeof(wpath[0]), path) < 0)
        {
            return -1;
        }

        return _wstat64(wpath, buf);
    }
#endif /* PLATFORM_ */
}

COM_UTIL_EXPORT int COM_UTIL_API com_util_mkdir(const char *path)
{
    if (path == NULL)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    return mkdir(path, 0755);
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wpath[PLATFORM_PATH_MAX];

        if (com_util_utf8_to_wpath(wpath, sizeof(wpath) / sizeof(wpath[0]), path) < 0)
        {
            return -1;
        }

        return _wmkdir(wpath);
    }
#endif /* PLATFORM_ */
}
