#include <testfw.h>
#include <mock_cplat.h>

cplat_trace_level delegate_real_cplat_tracer_get_file_level(cplat_tracer *handle)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_tracer_get_file_level)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_tracer_get_file_level"));

    return real_fn(handle);
}

MOCK_WEAK_IMPL(cplat_trace_level, cplat_tracer_get_file_level, cplat_tracer *handle)
{
    cplat_trace_level mock_ret = CPLAT_TRACE_LEVEL_NONE;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_tracer_get_file_level(handle);
    }
    else
    {
        mock_ret = delegate_real_cplat_tracer_get_file_level(handle);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (void *)handle);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", (int)mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
