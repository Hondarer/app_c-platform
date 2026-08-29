#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_get_realtime_deadline_ms(uint64_t timeout_ms, struct timespec *abs_timeout)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_get_realtime_deadline_ms)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_get_realtime_deadline_ms"));

    real_fn(timeout_ms, abs_timeout);
}

MOCK_WEAK_IMPL(void, cplat_get_realtime_deadline_ms, uint64_t timeout_ms, struct timespec *abs_timeout)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_get_realtime_deadline_ms(timeout_ms, abs_timeout);
    }
    else
    {
        delegate_real_cplat_get_realtime_deadline_ms(timeout_ms, abs_timeout);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %llu\n", __func__, (unsigned long long)timeout_ms);
    }
}
