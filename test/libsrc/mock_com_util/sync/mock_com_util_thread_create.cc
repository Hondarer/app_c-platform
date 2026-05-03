#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_thread_create(com_util_thread_t *thread, com_util_thread_func_t func, void *arg)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_thread_create)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_thread_create"));

    return real_fn(thread, func, arg);
}

WEAK_ATR int com_util_thread_create(com_util_thread_t *thread, com_util_thread_func_t func, void *arg)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_thread_create(thread, func, arg);
    }
    else
    {
        rtc = delegate_real_com_util_thread_create(thread, func, arg);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (void *)func);
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
