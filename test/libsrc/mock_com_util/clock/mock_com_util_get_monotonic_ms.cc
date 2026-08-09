#include <testfw.h>
#include <mock_com_util.h>

uint64_t delegate_real_com_util_get_monotonic_ms(void)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_get_monotonic_ms)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_get_monotonic_ms"));

    return real_fn();
}

MOCK_WEAK_IMPL(uint64_t, com_util_get_monotonic_ms, void)
{
    uint64_t rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_get_monotonic_ms();
    }
    else
    {
        rtc = delegate_real_com_util_get_monotonic_ms();
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %llu\n", (unsigned long long)rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}
