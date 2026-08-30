#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_shutdown_reset_for_test(void)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_shutdown_reset_for_test)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_shutdown_reset_for_test"));

    real_fn();
}

MOCK_WEAK_IMPL(void, cplat_shutdown_reset_for_test, void)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_shutdown_reset_for_test();
    }
    else
    {
        delegate_real_cplat_shutdown_reset_for_test();
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
