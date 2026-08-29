#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_timespec_to_native(const cplat_timespec *ts, struct timespec *native)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_timespec_to_native)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_timespec_to_native"));

    real_fn(ts, native);
}

MOCK_WEAK_IMPL(void, cplat_timespec_to_native, const cplat_timespec *ts, struct timespec *native)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_timespec_to_native(ts, native);
    }
    else
    {
        delegate_real_cplat_timespec_to_native(ts, native);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
