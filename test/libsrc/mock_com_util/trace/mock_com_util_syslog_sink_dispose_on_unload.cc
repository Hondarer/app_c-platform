#include <testfw.h>
#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)

#include <syslog_internal.h>

void com_util_syslog_sink_dispose_on_unload(com_util_syslog_sink_t *handle)
{
    (void)handle;
}

#endif /* PLATFORM_LINUX */
