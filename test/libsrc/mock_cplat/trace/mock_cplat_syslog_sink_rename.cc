#include <testfw.h>
#include <mock_cplat.h>

#if defined(PLATFORM_LINUX)

int delegate_real_cplat_syslog_sink_rename(cplat_syslog_sink *handle, const char *new_ident)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_syslog_sink_rename)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_syslog_sink_rename"));

    return real_fn(handle, new_ident);
}

MOCK_WEAK_IMPL(int, cplat_syslog_sink_rename, cplat_syslog_sink *handle, const char *new_ident)
{
    int mock_ret = -1;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_syslog_sink_rename(handle, new_ident);
    }
    else
    {
        mock_ret = delegate_real_cplat_syslog_sink_rename(handle, new_ident);
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
