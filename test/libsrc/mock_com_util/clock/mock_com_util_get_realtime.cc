#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_get_realtime(com_util_timespec *ts)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_get_realtime)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_get_realtime"));

    real_fn(ts);
}

MOCK_WEAK_IMPL(void, com_util_get_realtime, com_util_timespec *ts)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_get_realtime(ts);
    }
    else
    {
        delegate_real_com_util_get_realtime(ts);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
