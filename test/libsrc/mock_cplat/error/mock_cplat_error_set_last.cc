#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_error_set_last(const cplat_error *error)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_error_set_last)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_error_set_last"));

    real_fn(error);
}

MOCK_WEAK_IMPL(void, cplat_error_set_last, const cplat_error *error)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_error_set_last(error);
    }
    else
    {
        delegate_real_cplat_error_set_last(error);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
