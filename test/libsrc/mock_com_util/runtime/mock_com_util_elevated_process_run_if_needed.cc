#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_elevated_process_run_if_needed(const char *arguments, int *exit_code, int *handled)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_elevated_process_run_if_needed)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_elevated_process_run_if_needed"));

    return real_fn(arguments, exit_code, handled);
}

MOCK_WEAK_IMPL(int, com_util_elevated_process_run_if_needed, const char *arguments, int *exit_code, int *handled)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_elevated_process_run_if_needed(arguments, exit_code, handled);
    }
    else
    {
        rtc = delegate_real_com_util_elevated_process_run_if_needed(arguments, exit_code, handled);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s ", __func__);
        if (arguments != nullptr)
        {
            printf("%s", arguments);
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
