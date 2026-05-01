#include <com_util/crt/sys/stat.h>
#include <com_util/crt/path.h>

#include <com_util/crt/path_format.h>

COM_UTIL_EXPORT int COM_UTIL_API com_util_vstat_fmt(com_util_file_stat_t *buf,
                                                     const char       *format,
                                                     va_list           args)
{
    char filename[PLATFORM_PATH_MAX] = {0};

    if (buf == NULL)
    {
        return -1;
    }

    if (com_util_vformat_path(filename, sizeof(filename), format, args, NULL) != 0)
    {
        return -1;
    }

    return com_util_stat(buf, filename);
}

COM_UTIL_EXPORT int COM_UTIL_API com_util_stat_fmt(com_util_file_stat_t *buf,
                                                    const char       *format,
                                                    ...)
{
    int     result;
    va_list args;

    va_start(args, format);
    result = com_util_vstat_fmt(buf, format, args);
    va_end(args);

    return result;
}

COM_UTIL_EXPORT int COM_UTIL_API com_util_vmkdir_fmt(const char *format,
                                                      va_list     args)
{
    char filename[PLATFORM_PATH_MAX] = {0};

    if (com_util_vformat_path(filename, sizeof(filename), format, args, NULL) != 0)
    {
        return -1;
    }

    return com_util_mkdir(filename);
}

COM_UTIL_EXPORT int COM_UTIL_API com_util_mkdir_fmt(const char *format, ...)
{
    int     result;
    va_list args;

    va_start(args, format);
    result = com_util_vmkdir_fmt(format, args);
    va_end(args);

    return result;
}
