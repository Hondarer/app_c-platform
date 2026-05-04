#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_thread_join_timed(com_util_thread_t *thread, uint32_t timeout_ms)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_thread_join_timed)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_thread_join_timed"));

    return real_fn(thread, timeout_ms);
}

MOCK_WEAK_IMPL(int, com_util_thread_join_timed, com_util_thread_t *thread, uint32_t timeout_ms)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_thread_join_timed(thread, timeout_ms);
    }
    else
    {
        rtc = delegate_real_com_util_thread_join_timed(thread, timeout_ms);
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
