#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_prompt_dispose(cplat_prompt *prompt)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_prompt_dispose)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_prompt_dispose"));

    real_fn(prompt);
}

MOCK_WEAK_IMPL(void, cplat_prompt_dispose, cplat_prompt *prompt)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_prompt_dispose(prompt);
    }
    else
    {
        delegate_real_cplat_prompt_dispose(prompt);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p\n", __func__, (void *)prompt);
    }
}
