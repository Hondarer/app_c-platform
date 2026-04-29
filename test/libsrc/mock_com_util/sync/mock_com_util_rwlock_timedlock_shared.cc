#include <testfw.h>
#include <mock_com_util.h>

WEAK_ATR int com_util_rwlock_timedlock_shared(com_util_rwlock_t *rwlock, uint32_t timeout_ms)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_rwlock_timedlock_shared(rwlock, timeout_ms);
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
