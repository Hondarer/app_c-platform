#include <testfw.h>
#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)

#include <com_util/trace/backends/etw/etw_internal.h>

void com_util_etw_provider_dispose_on_shutdown(com_util_etw_provider_t *handle,
                                               const com_util_shutdown_event_t *event)
{
    (void)handle;
    (void)event;
}

#endif /* PLATFORM_WINDOWS */
