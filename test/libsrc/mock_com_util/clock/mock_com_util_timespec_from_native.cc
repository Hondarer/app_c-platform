#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_timespec_from_native(const struct timespec *native, com_util_timespec *ts)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_timespec_from_native)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_timespec_from_native"));

    real_fn(native, ts);
}

MOCK_WEAK_IMPL(void, com_util_timespec_from_native, const struct timespec *native, com_util_timespec *ts)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_timespec_from_native(native, ts);
    }
    else
    {
        delegate_real_com_util_timespec_from_native(native, ts);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
