#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_LINUX)

com_util_syslog_sink *delegate_real_com_util_syslog_sink_create(const char *ident, int facility)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_syslog_sink_create)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_syslog_sink_create"));

    return real_fn(ident, facility);
}

MOCK_WEAK_IMPL(com_util_syslog_sink *, com_util_syslog_sink_create, const char *ident, int facility)
{
    com_util_syslog_sink *mock_ret = nullptr;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_syslog_sink_create(ident, facility);
    }
    else
    {
        mock_ret = delegate_real_com_util_syslog_sink_create(ident, facility);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s \"%s\"", __func__, ident != nullptr ? ident : "(null)");
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}

#endif /* PLATFORM_LINUX */
