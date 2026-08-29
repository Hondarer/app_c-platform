#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_tracer_dispose(cplat_tracer **handle)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_tracer_dispose)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_tracer_dispose"));

    real_fn(handle);
}

MOCK_WEAK_IMPL(void, cplat_tracer_dispose, cplat_tracer **handle)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_tracer_dispose(handle);
    }
    else
    {
        delegate_real_cplat_tracer_dispose(handle);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p\n", __func__, (void *)handle);
    }
}
