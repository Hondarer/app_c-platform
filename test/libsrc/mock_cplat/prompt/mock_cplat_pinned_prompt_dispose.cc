#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_pinned_prompt_dispose(cplat_pinned_prompt *screen)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_pinned_prompt_dispose)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_pinned_prompt_dispose"));

    real_fn(screen);
}

MOCK_WEAK_IMPL(void, cplat_pinned_prompt_dispose, cplat_pinned_prompt *screen)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_pinned_prompt_dispose(screen);
    }
    else
    {
        delegate_real_cplat_pinned_prompt_dispose(screen);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p\n", __func__, (void *)screen);
    }
}
