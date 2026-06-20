#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_elevated_process_report_result(const char *message)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_elevated_process_report_result)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_elevated_process_report_result"));

    return real_fn(message);
}

MOCK_WEAK_IMPL(int, com_util_elevated_process_report_result, const char *message)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_elevated_process_report_result(message);
    }
    else
    {
        rtc = delegate_real_com_util_elevated_process_report_result(message);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s ", __func__);
        if (message != nullptr)
        {
            printf("%s", message);
        }
        else
        {
            printf("(null)");
        }
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
