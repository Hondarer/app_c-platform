#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

int delegate_real_com_util_eventlog_sink_write(com_util_eventlog_sink *handle, int level, const char *instance_name,
                                               const char *message)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_eventlog_sink_write)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_eventlog_sink_write"));

    return real_fn(handle, level, instance_name, message);
}

MOCK_WEAK_IMPL(int, com_util_eventlog_sink_write, com_util_eventlog_sink *handle, int level, const char *instance_name,
               const char *message)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_eventlog_sink_write(handle, level, instance_name, message);
    }
    else
    {
        rtc = delegate_real_com_util_eventlog_sink_write(handle, level, instance_name, message);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d \"%s\"", __func__, level, message != nullptr ? message : "(null)");
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}

#endif /* PLATFORM_WINDOWS */
