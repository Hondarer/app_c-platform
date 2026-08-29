#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_timespec_from_native(const struct timespec *native, cplat_timespec *ts)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_timespec_from_native)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_timespec_from_native"));

    real_fn(native, ts);
}

MOCK_WEAK_IMPL(void, cplat_timespec_from_native, const struct timespec *native, cplat_timespec *ts)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_timespec_from_native(native, ts);
    }
    else
    {
        delegate_real_cplat_timespec_from_native(native, ts);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
