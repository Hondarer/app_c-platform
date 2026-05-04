#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_get_realtime_deadline_ms(uint64_t timeout_ms, struct timespec *abs_timeout)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_get_realtime_deadline_ms)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_get_realtime_deadline_ms"));

    real_fn(timeout_ms, abs_timeout);
}

MOCK_WEAK_IMPL(void, com_util_get_realtime_deadline_ms, uint64_t timeout_ms, struct timespec *abs_timeout)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_get_realtime_deadline_ms(timeout_ms, abs_timeout);
    }
    else
    {
        delegate_real_com_util_get_realtime_deadline_ms(timeout_ms, abs_timeout);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %llu\n", __func__, (unsigned long long)timeout_ms);
    }
}
