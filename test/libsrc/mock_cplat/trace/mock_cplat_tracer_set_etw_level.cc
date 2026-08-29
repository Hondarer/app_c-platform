#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_tracer_set_etw_level(cplat_tracer *handle, cplat_trace_level level)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_tracer_set_etw_level)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_tracer_set_etw_level"));

    return real_fn(handle, level);
}

MOCK_WEAK_IMPL(int, cplat_tracer_set_etw_level, cplat_tracer *handle, cplat_trace_level level)
{
    int mock_ret = 0;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_tracer_set_etw_level(handle, level);
    }
    else
    {
        mock_ret = delegate_real_cplat_tracer_set_etw_level(handle, level);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %d", __func__, (void *)handle, (int)level);
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
