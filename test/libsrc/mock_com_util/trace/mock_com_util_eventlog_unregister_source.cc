#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

int delegate_real_com_util_eventlog_unregister_source(const char *source_name)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_eventlog_unregister_source)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_eventlog_unregister_source"));

    return real_fn(source_name);
}

MOCK_WEAK_IMPL(int, com_util_eventlog_unregister_source, const char *source_name)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_eventlog_unregister_source(source_name);
    }
    else
    {
        rtc = delegate_real_com_util_eventlog_unregister_source(source_name);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s \"%s\"", __func__, source_name != nullptr ? source_name : "(null)");
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
