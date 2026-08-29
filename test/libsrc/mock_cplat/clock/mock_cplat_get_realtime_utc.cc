#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_get_realtime_utc(struct tm *utc_tm, int32_t *tv_nsec)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_get_realtime_utc)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_get_realtime_utc"));

    real_fn(utc_tm, tv_nsec);
}

MOCK_WEAK_IMPL(void, cplat_get_realtime_utc, struct tm *utc_tm, int32_t *tv_nsec)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_get_realtime_utc(utc_tm, tv_nsec);
    }
    else
    {
        delegate_real_cplat_get_realtime_utc(utc_tm, tv_nsec);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
