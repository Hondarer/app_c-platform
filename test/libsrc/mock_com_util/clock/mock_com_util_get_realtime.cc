#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_get_realtime(int64_t *tv_sec, int32_t *tv_nsec)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_get_realtime)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_get_realtime"));

    real_fn(tv_sec, tv_nsec);
}

MOCK_WEAK_IMPL(void, com_util_get_realtime, int64_t *tv_sec, int32_t *tv_nsec)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_get_realtime(tv_sec, tv_nsec);
    }
    else
    {
        delegate_real_com_util_get_realtime(tv_sec, tv_nsec);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
