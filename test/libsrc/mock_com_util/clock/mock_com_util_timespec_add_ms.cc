#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_timespec_add_ms(const com_util_timespec *ts, uint64_t timeout_ms, com_util_timespec *result)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_timespec_add_ms)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_timespec_add_ms"));

    real_fn(ts, timeout_ms, result);
}

MOCK_WEAK_IMPL(void, com_util_timespec_add_ms, const com_util_timespec *ts, uint64_t timeout_ms,
               com_util_timespec *result)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_timespec_add_ms(ts, timeout_ms, result);
    }
    else
    {
        delegate_real_com_util_timespec_add_ms(ts, timeout_ms, result);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %llu\n", __func__, (unsigned long long)timeout_ms);
    }
}
