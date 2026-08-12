#ifndef TRACE_FILE_INJECT_H
#define TRACE_FILE_INJECT_H

#include <stddef.h>

#include <com_util/trace/trace_file.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    extern com_util_trace_file_sink *test_trace_file_sink_create_unregistered(const char *path,
                                                                               size_t max_bytes,
                                                                               int generations,
                                                                               int flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* TRACE_FILE_INJECT_H */
