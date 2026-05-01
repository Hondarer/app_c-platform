#include <com_util/crt/unistd.h>
#include <com_util/crt/path.h>

#include <com_util/crt/path_format.h>

COM_UTIL_EXPORT int COM_UTIL_API com_util_vaccess_fmt(int mode,
                                                       const char *format,
                                                       va_list args)
{
    char filename[PLATFORM_PATH_MAX] = {0};

    if (com_util_vformat_path(filename, sizeof(filename), format, args, NULL) != 0)
    {
        return -1;
    }

    return com_util_access(filename, mode);
}

COM_UTIL_EXPORT int COM_UTIL_API com_util_access_fmt(int mode,
                                                      const char *format,
                                                      ...)
{
    int     result;
    va_list args;

    va_start(args, format);
    result = com_util_vaccess_fmt(mode, format, args);
    va_end(args);

    return result;
}
