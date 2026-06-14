#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

int delegate_real_com_util_eventlog_sink_write(com_util_eventlog_sink *handle, int level, int64_t file_identifier,
                                               const char *instance_name, int64_t instance_identifier,
                                               const char *message)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_eventlog_sink_write)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_eventlog_sink_write"));

    return real_fn(handle, level, file_identifier, instance_name, instance_identifier, message);
}

MOCK_WEAK_IMPL(int, com_util_eventlog_sink_write, com_util_eventlog_sink *handle, int level, int64_t file_identifier,
               const char *instance_name, int64_t instance_identifier, const char *message)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_eventlog_sink_write(handle, level, file_identifier, instance_name,
                                                           instance_identifier, message);
    }
    else
    {
        rtc = delegate_real_com_util_eventlog_sink_write(handle, level, file_identifier, instance_name,
                                                         instance_identifier, message);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d %lld \"%s\" %lld \"%s\"", __func__, level, (long long)file_identifier,
               instance_name != nullptr ? instance_name : "(null)", (long long)instance_identifier,
               message != nullptr ? message : "(null)");
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
