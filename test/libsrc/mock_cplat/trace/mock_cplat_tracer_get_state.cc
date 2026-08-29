#include <testfw.h>
#include <mock_cplat.h>

cplat_tracer_state delegate_real_cplat_tracer_get_state(cplat_tracer *handle)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_tracer_get_state)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_tracer_get_state"));

    return real_fn(handle);
}

MOCK_WEAK_IMPL(cplat_tracer_state, cplat_tracer_get_state, cplat_tracer *handle)
{
    cplat_tracer_state mock_ret = CPLAT_TRACER_STATE_DISPOSED;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_tracer_get_state(handle);
    }
    else
    {
        mock_ret = delegate_real_cplat_tracer_get_state(handle);
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
