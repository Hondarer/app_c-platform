#include <testfw.h>
#include <mock_com_util.h>

WEAK_ATR int com_util_condvar_wait(com_util_condvar_t *cv, com_util_mutex_t *mtx)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_condvar_wait(cv, mtx);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, 0x%p", __func__, (void *)cv, (void *)mtx);
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
