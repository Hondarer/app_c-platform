#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_timespec_to_native(const com_util_timespec *ts, struct timespec *native)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_timespec_to_native)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_timespec_to_native"));

    real_fn(ts, native);
}

MOCK_WEAK_IMPL(void, com_util_timespec_to_native, const com_util_timespec *ts, struct timespec *native)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_timespec_to_native(ts, native);
    }
    else
    {
        delegate_real_com_util_timespec_to_native(ts, native);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
