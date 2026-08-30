#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_error_clear_last(void)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_error_clear_last)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_error_clear_last"));

    real_fn();
}

MOCK_WEAK_IMPL(void, cplat_error_clear_last, void)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_error_clear_last();
    }
    else
    {
        delegate_real_cplat_error_clear_last();
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
