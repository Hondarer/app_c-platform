#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_rwlock_timedlock_shared(com_util_rwlock_t *rwlock, uint32_t timeout_ms)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_rwlock_timedlock_shared)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_rwlock_timedlock_shared"));

    return real_fn(rwlock, timeout_ms);
}

WEAK_ATR int com_util_rwlock_timedlock_shared(com_util_rwlock_t *rwlock, uint32_t timeout_ms)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_rwlock_timedlock_shared(rwlock, timeout_ms);
    }
    else
    {
        rtc = delegate_real_com_util_rwlock_timedlock_shared(rwlock, timeout_ms);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %u", __func__, (void *)rwlock, timeout_ms);
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
