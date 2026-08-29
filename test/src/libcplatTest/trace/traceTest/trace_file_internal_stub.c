/** tracer 単体テストで内部ファイル出力を公開 mock へ中継します。 */

#include <cplat/trace/trace_file.h>
#include <cplat/trace/backends/file/trace_file_internal.h>

int cplat_internal_trace_file_sink_write_text(cplat_trace_file_sink *handle, const int level,
                                              const cplat_timespec *timestamp, const char *timestamp_text,
                                              const char *message)
{
    (void)timestamp_text;
    return cplat_trace_file_sink_write(handle, level, timestamp, message);
}
