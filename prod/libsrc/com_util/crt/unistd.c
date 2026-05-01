#include <com_util/crt/unistd.h>
#include <com_util/crt/path.h>

#include <com_util/crt/crt_internal.h>

COM_UTIL_EXPORT int COM_UTIL_API com_util_access(const char *path,
                                                  int         mode)
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
