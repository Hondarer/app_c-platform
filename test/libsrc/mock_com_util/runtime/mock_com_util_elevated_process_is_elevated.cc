#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_elevated_process_is_elevated(int *elevated)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_elevated_process_is_elevated)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_elevated_process_is_elevated"));

    return real_fn(elevated);
}

MOCK_WEAK_IMPL(int, com_util_elevated_process_is_elevated, int *elevated)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_elevated_process_is_elevated(elevated);
    }
    else
    {
        rtc = delegate_real_com_util_elevated_process_is_elevated(elevated);
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
