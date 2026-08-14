#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_LINUX)

int delegate_real_com_util_syslog_sink_rename(com_util_syslog_sink *handle, const char *new_ident)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_syslog_sink_rename)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_syslog_sink_rename"));

    return real_fn(handle, new_ident);
}

MOCK_WEAK_IMPL(int, com_util_syslog_sink_rename, com_util_syslog_sink *handle, const char *new_ident)
{
    int mock_ret = -1;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_syslog_sink_rename(handle, new_ident);
    }
    else
    {
        mock_ret = delegate_real_com_util_syslog_sink_rename(handle, new_ident);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s \"%s\"", __func__, new_ident != nullptr ? new_ident : "(null)");
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}

#endif /* PLATFORM_LINUX */
