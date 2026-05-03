#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_console_init(void)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_console_init)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_console_init"));

    real_fn();
}

WEAK_ATR void com_util_console_init(void)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_console_init();
    }
    else
    {
        delegate_real_com_util_console_init();
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
