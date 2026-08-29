#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_elevated_process_extract_result_target(int *argc, char **argv, int *detected_out)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_elevated_process_extract_result_target)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_elevated_process_extract_result_target"));

    return real_fn(argc, argv, detected_out);
}

MOCK_WEAK_IMPL(int, cplat_elevated_process_extract_result_target, int *argc, char **argv, int *detected_out)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_elevated_process_extract_result_target(argc, argv, detected_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_elevated_process_extract_result_target(argc, argv, detected_out);
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
