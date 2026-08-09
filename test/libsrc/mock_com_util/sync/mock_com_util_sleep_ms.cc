#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_sleep_ms(int ms)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_sleep_ms)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_sleep_ms"));

    real_fn(ms);
}

MOCK_WEAK_IMPL(void, com_util_sleep_ms, int ms)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_sleep_ms(ms);
    }
    else
    {
        delegate_real_com_util_sleep_ms(ms);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d\n", __func__, ms);
    }
}
