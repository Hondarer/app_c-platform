#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_tracer_remove_hook(cplat_tracer *handle, cplat_tracer_hook_entry *hook_entry)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_tracer_remove_hook)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_tracer_remove_hook"));

    real_fn(handle, hook_entry);
}

MOCK_WEAK_IMPL(void, cplat_tracer_remove_hook, cplat_tracer *handle, cplat_tracer_hook_entry *hook_entry)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_tracer_remove_hook(handle, hook_entry);
    }
    else
    {
        delegate_real_cplat_tracer_remove_hook(handle, hook_entry);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, 0x%p\n", __func__, (void *)handle, (void *)hook_entry);
    }
}
