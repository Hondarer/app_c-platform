#include <testfw.h>
#include <com_util/trace/backends/file/trace_file_internal.h>

void (*g_test_file_shutdown_hook)(void) = NULL;

void com_util_trace_file_sink_dispose_on_shutdown(com_util_trace_file_sink *handle)
{
    (void)handle;
    if (g_test_file_shutdown_hook != NULL)
    {
        g_test_file_shutdown_hook();
    }
}
