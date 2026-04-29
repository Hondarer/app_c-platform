#include <testfw.h>
#include <mock_com_util.h>

WEAK_ATR int com_util_rwlock_unlock_exclusive(com_util_rwlock_t *rwlock)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_rwlock_unlock_exclusive(rwlock);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (void *)rwlock);
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
