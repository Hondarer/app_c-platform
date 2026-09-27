#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_clock_get_monotonic(cplat_timespec *ts)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_clock_get_monotonic)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_clock_get_monotonic"));

    real_fn(ts);
}

MOCK_WEAK_IMPL(void, cplat_clock_get_monotonic, cplat_timespec *ts)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_clock_get_monotonic(ts);
    }
    else
    {
        delegate_real_cplat_clock_get_monotonic(ts);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
