#include <testfw.h>
#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)

    #include <com_util/trace/backends/syslog/syslog_internal.h>

void com_util_syslog_sink_dispose_on_shutdown(com_util_syslog_sink *handle)
{
    (void)handle;
}

#endif /* PLATFORM_LINUX */
