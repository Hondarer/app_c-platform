#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

com_util_eventlog_sink *delegate_real_com_util_eventlog_sink_create(const char *source_name)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_eventlog_sink_create)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_eventlog_sink_create"));

    return real_fn(source_name);
}

MOCK_WEAK_IMPL(com_util_eventlog_sink *, com_util_eventlog_sink_create, const char *source_name)
{
    com_util_eventlog_sink *rtc = nullptr;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_eventlog_sink_create(source_name);
    }
    else
    {
        rtc = delegate_real_com_util_eventlog_sink_create(source_name);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s \"%s\"", __func__, source_name != nullptr ? source_name : "(null)");
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}

#endif /* PLATFORM_WINDOWS */
