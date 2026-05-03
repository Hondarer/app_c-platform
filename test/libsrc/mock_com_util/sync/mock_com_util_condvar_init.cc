#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_condvar_init(com_util_condvar_t *cv)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_condvar_init)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_condvar_init"));

    return real_fn(cv);
}

WEAK_ATR int com_util_condvar_init(com_util_condvar_t *cv)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_condvar_init(cv);
    }
    else
    {
        rtc = delegate_real_com_util_condvar_init(cv);
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
