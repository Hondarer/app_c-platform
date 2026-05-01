#include <testfw.h>
#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)

#include <com_util/trace/backends/etw/etw_internal.h>

void com_util_etw_provider_dispose_on_unload(com_util_etw_provider_t *handle, int process_terminating)
{
    (void)handle;
    (void)process_terminating;
}

#endif /* PLATFORM_WINDOWS */
