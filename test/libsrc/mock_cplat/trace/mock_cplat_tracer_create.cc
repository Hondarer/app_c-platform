#include <testfw.h>
#include <mock_cplat.h>

cplat_tracer *delegate_real_cplat_tracer_create(const cplat_tracer_concurrency_mode concurrency_mode)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_tracer_create)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_tracer_create"));

    return real_fn(concurrency_mode);
}

MOCK_WEAK_IMPL(cplat_tracer *, cplat_tracer_create, cplat_tracer_concurrency_mode concurrency_mode)
{
    cplat_tracer *mock_ret = nullptr;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_tracer_create(concurrency_mode);
    }
    else
    {
        mock_ret = delegate_real_cplat_tracer_create(concurrency_mode);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
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
