#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_shutdown_register(com_util_shutdown_fn callback, void *context)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_shutdown_register)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_shutdown_register"));

    return real_fn(callback, context);
}

MOCK_WEAK_IMPL(int, com_util_shutdown_register, com_util_shutdown_fn callback, void *context)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_shutdown_register(callback, context);
    }
    else
    {
        rtc = delegate_real_com_util_shutdown_register(callback, context);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
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
