#include "trace_file.inject.h"

#include <string.h>

com_util_trace_file_sink *test_trace_file_sink_create_unregistered(const char *path, const size_t max_bytes,
                                                                   const int generations, const int flags)
{
    return create_new_sink(path, strlen(path), max_bytes, generations, flags);
}
