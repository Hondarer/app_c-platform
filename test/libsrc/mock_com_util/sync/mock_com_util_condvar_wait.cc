#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_condvar_wait(com_util_condvar_t *cv, com_util_mutex_t *mtx)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_condvar_wait)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_condvar_wait"));

    return real_fn(cv, mtx);
}

WEAK_ATR int com_util_condvar_wait(com_util_condvar_t *cv, com_util_mutex_t *mtx)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_condvar_wait(cv, mtx);
    }
    else
    {
        rtc = delegate_real_com_util_condvar_wait(cv, mtx);
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
