#include <testfw.h>
#include <cplat/base/platform.h>

#if defined(PLATFORM_WINDOWS)

    #include <cplat/trace/backends/etw/etw_internal.h>

void cplat_etw_provider_dispose_on_shutdown(cplat_etw_provider *handle, const cplat_shutdown_event *event)
{
    (void)handle;
    (void)event;
}

#endif /* PLATFORM_WINDOWS */
