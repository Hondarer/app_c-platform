#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_elevated_process_report_result(const char *message)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_elevated_process_report_result)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_elevated_process_report_result"));

    return real_fn(message);
}

MOCK_WEAK_IMPL(int, cplat_elevated_process_report_result, const char *message)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_elevated_process_report_result(message);
    }
    else
    {
        mock_ret = delegate_real_cplat_elevated_process_report_result(message);
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
