#include <testfw.h>
#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)

    #include <com_util/trace/backends/eventlog/eventlog_internal.h>

void com_util_eventlog_sink_dispose_on_shutdown(com_util_eventlog_sink *handle, const com_util_shutdown_event *event)
{
    (void)handle;
    (void)event;
}

#endif /* PLATFORM_WINDOWS */
