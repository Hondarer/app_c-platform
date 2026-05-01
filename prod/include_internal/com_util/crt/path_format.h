#ifndef COM_UTIL_CRT_PATH_FORMAT_INTERNAL_H
#define COM_UTIL_CRT_PATH_FORMAT_INTERNAL_H

#include <stdarg.h>
#include <stddef.h>

int com_util_vformat_path(char *path,
                          size_t path_size,
                          const char *format,
                          va_list args,
                          int *error_out);

#endif /* COM_UTIL_CRT_PATH_FORMAT_INTERNAL_H */
