#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

int delegate_real_com_util_etw_session_check_access(void)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_etw_session_check_access)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_etw_session_check_access"));

    return real_fn();
}

MOCK_WEAK_IMPL(int, com_util_etw_session_check_access, void)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_etw_session_check_access();
    }
    else
    {
        rtc = delegate_real_com_util_etw_session_check_access();
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

#endif /* PLATFORM_WINDOWS */
