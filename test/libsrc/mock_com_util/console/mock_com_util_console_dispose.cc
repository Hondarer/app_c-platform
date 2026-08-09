#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_console_dispose(void)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_console_dispose)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_console_dispose"));

    real_fn();
}

MOCK_WEAK_IMPL(void, com_util_console_dispose, void)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_console_dispose();
    }
    else
    {
        delegate_real_com_util_console_dispose();
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
