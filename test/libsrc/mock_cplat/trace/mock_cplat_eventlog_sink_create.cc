#include <testfw.h>
#include <mock_cplat.h>

#if defined(PLATFORM_WINDOWS)

cplat_eventlog_sink *delegate_real_cplat_eventlog_sink_create(const char *source_name)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_eventlog_sink_create)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_eventlog_sink_create"));

    return real_fn(source_name);
}

MOCK_WEAK_IMPL(cplat_eventlog_sink *, cplat_eventlog_sink_create, const char *source_name)
{
    cplat_eventlog_sink *mock_ret = nullptr;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_eventlog_sink_create(source_name);
    }
    else
    {
        mock_ret = delegate_real_cplat_eventlog_sink_create(source_name);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s \"%s\"", __func__, source_name != nullptr ? source_name : "(null)");
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

#endif /* PLATFORM_WINDOWS */
