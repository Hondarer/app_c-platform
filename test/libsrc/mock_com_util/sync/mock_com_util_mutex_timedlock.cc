#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_mutex_timedlock(com_util_mutex_t *mtx, uint32_t timeout_ms)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_mutex_timedlock)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_mutex_timedlock"));

    return real_fn(mtx, timeout_ms);
}

MOCK_WEAK_IMPL(int, com_util_mutex_timedlock, com_util_mutex_t *mtx, uint32_t timeout_ms)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_mutex_timedlock(mtx, timeout_ms);
    }
    else
    {
        rtc = delegate_real_com_util_mutex_timedlock(mtx, timeout_ms);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %u", __func__, (void *)mtx, timeout_ms);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}
