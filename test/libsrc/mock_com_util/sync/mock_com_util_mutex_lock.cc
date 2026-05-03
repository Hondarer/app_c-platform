#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_mutex_lock(com_util_mutex_t *mtx)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_mutex_lock)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_mutex_lock"));

    return real_fn(mtx);
}

WEAK_ATR int com_util_mutex_lock(com_util_mutex_t *mtx)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_mutex_lock(mtx);
    }
    else
    {
        rtc = delegate_real_com_util_mutex_lock(mtx);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (void *)mtx);
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
