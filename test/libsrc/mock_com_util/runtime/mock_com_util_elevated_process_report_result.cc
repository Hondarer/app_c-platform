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
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_elevated_process_report_result(message);
    }
    else
    {
        mock_ret = delegate_real_com_util_elevated_process_report_result(message);
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
            printf(" -> %d\n", mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
