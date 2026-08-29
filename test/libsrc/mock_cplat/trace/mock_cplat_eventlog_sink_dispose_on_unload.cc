#include <testfw.h>
#include <cplat/base/platform.h>

#if defined(PLATFORM_WINDOWS)

    #include <cplat/trace/backends/eventlog/eventlog_internal.h>

void cplat_eventlog_sink_dispose_on_shutdown(cplat_eventlog_sink *handle, const cplat_shutdown_event *event)
{
    (void)handle;
    (void)event;
}

#endif /* PLATFORM_WINDOWS */
