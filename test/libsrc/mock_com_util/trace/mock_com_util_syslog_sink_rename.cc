#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_LINUX)

int delegate_real_com_util_syslog_sink_rename(com_util_syslog_sink_t *handle, const char *new_ident)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_syslog_sink_rename)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_syslog_sink_rename"));

    return real_fn(handle, new_ident);
}

MOCK_WEAK_IMPL(int, com_util_syslog_sink_rename, com_util_syslog_sink_t *handle, const char *new_ident)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_syslog_sink_rename(handle, new_ident);
    }
    else
    {
        rtc = delegate_real_com_util_syslog_sink_rename(handle, new_ident);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s \"%s\"", __func__, new_ident != nullptr ? new_ident : "(null)");
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

#endif /* PLATFORM_LINUX */
