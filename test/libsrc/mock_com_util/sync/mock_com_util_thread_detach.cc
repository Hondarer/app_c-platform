#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_thread_detach(com_util_thread_t *thread)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_thread_detach)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_thread_detach"));

    return real_fn(thread);
}

WEAK_ATR int com_util_thread_detach(com_util_thread_t *thread)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_thread_detach(thread);
    }
    else
    {
        rtc = delegate_real_com_util_thread_detach(thread);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (void *)thread);
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
