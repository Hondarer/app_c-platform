#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_gmtime(struct tm *utc_tm, const time_t *timep)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_gmtime)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_gmtime"));

    return real_fn(utc_tm, timep);
}

MOCK_WEAK_IMPL(int, com_util_gmtime, struct tm *utc_tm, const time_t *timep)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_gmtime(utc_tm, timep);
    }
    else
    {
        rtc = delegate_real_com_util_gmtime(utc_tm, timep);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, 0x%p", __func__, (void *)utc_tm, (const void *)timep);
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
