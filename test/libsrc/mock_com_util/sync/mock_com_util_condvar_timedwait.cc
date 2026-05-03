#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_condvar_timedwait(com_util_condvar_t *cv, com_util_mutex_t *mtx, uint32_t timeout_ms)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_condvar_timedwait)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_condvar_timedwait"));

    return real_fn(cv, mtx, timeout_ms);
}

WEAK_ATR int com_util_condvar_timedwait(com_util_condvar_t *cv, com_util_mutex_t *mtx, uint32_t timeout_ms)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_condvar_timedwait(cv, mtx, timeout_ms);
    }
    else
    {
        rtc = delegate_real_com_util_condvar_timedwait(cv, mtx, timeout_ms);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %u", __func__, timeout_ms);
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
