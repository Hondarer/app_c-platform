#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_thread_join(com_util_thread_t *thread)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_thread_join)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_thread_join"));

    real_fn(thread);
}

WEAK_ATR void com_util_thread_join(com_util_thread_t *thread)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_thread_join(thread);
    }
    else
    {
        delegate_real_com_util_thread_join(thread);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
