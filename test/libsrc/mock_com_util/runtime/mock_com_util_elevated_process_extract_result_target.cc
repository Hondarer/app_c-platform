#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_elevated_process_extract_result_target(int *argc, char **argv)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_elevated_process_extract_result_target)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_elevated_process_extract_result_target"));

    return real_fn(argc, argv);
}

MOCK_WEAK_IMPL(int, com_util_elevated_process_extract_result_target, int *argc, char **argv)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_elevated_process_extract_result_target(argc, argv);
    }
    else
    {
        rtc = delegate_real_com_util_elevated_process_extract_result_target(argc, argv);
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
