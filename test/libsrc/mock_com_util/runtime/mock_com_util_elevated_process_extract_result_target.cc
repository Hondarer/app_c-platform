#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_elevated_process_extract_result_target(int *argc, char **argv, int *detected_out)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_elevated_process_extract_result_target)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_elevated_process_extract_result_target"));

    return real_fn(argc, argv, detected_out);
}

MOCK_WEAK_IMPL(int, com_util_elevated_process_extract_result_target, int *argc, char **argv, int *detected_out)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_elevated_process_extract_result_target(argc, argv, detected_out);
    }
    else
    {
        mock_ret = delegate_real_com_util_elevated_process_extract_result_target(argc, argv, detected_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
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
