#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_condvar_signal(com_util_condvar_t *cv)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_condvar_signal)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_condvar_signal"));

    return real_fn(cv);
}

WEAK_ATR int com_util_condvar_signal(com_util_condvar_t *cv)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_condvar_signal(cv);
    }
    else
    {
        rtc = delegate_real_com_util_condvar_signal(cv);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (void *)cv);
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
