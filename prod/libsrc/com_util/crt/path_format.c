#include <com_util/crt/path_format.h>

#include <errno.h>
#include <stdio.h>

int com_util_vformat_path(char *path, const size_t path_size, const char *format, va_list args, int *error_out)
{
    int written;

    if (path == NULL || path_size == 0 || format == NULL)
    {
        if (error_out != NULL)
        {
            *error_out = EINVAL;
        }
        return -1;
    }

    written = vsnprintf(path, path_size, format, args);
    if (written < 0)
    {
        if (error_out != NULL)
        {
            *error_out = EINVAL;
        }
        return -1;
    }

    if (written >= (int)path_size)
    {
        if (error_out != NULL)
        {
            *error_out = ENAMETOOLONG;
        }
        return -1;
    }

    return 0;
}
