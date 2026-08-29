#include <testfw.h>
#include <mock_cplat.h>

#if defined(PLATFORM_WINDOWS)

int delegate_real_cplat_eventlog_unregister_source(const char *source_name)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_eventlog_unregister_source)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_eventlog_unregister_source"));

    return real_fn(source_name);
}

MOCK_WEAK_IMPL(int, cplat_eventlog_unregister_source, const char *source_name)
{
    int mock_ret = -1;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_eventlog_unregister_source(source_name);
    }
    else
    {
        mock_ret = delegate_real_cplat_eventlog_unregister_source(source_name);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s \"%s\"", __func__, source_name != nullptr ? source_name : "(null)");
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

#endif /* PLATFORM_WINDOWS */
