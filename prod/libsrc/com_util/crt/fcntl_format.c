#include <com_util/crt/fcntl.h>
#include <com_util/crt/path.h>

#include <com_util/crt/path_format.h>

COM_UTIL_EXPORT int COM_UTIL_API com_util_vopen_fmt(int flags,
                                                     int mode,
                                                     const char *format,
                                                     va_list args)
{
    char filename[PLATFORM_PATH_MAX] = {0};

    if (com_util_vformat_path(filename, sizeof(filename), format, args, NULL) != 0)
    {
        return -1;
    }

    return com_util_open(filename, flags, mode);
}

COM_UTIL_EXPORT int COM_UTIL_API com_util_open_fmt(int flags,
                                                    int mode,
                                                    const char *format,
                                                    ...)
{
    int     result;
    va_list args;

    va_start(args, format);
    result = com_util_vopen_fmt(flags, mode, format, args);
    va_end(args);

    return result;
}
