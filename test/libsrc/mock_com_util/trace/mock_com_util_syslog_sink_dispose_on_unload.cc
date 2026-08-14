#include <testfw.h>
#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)

    #include <com_util/trace/backends/syslog/syslog_internal.h>

void (*g_test_syslog_shutdown_hook)(void) = NULL;

void com_util_syslog_sink_dispose_on_shutdown(com_util_syslog_sink *handle)
{
    (void)handle;
    if (g_test_syslog_shutdown_hook != NULL)
    {
        g_test_syslog_shutdown_hook();
    }
}

#endif /* PLATFORM_LINUX */
