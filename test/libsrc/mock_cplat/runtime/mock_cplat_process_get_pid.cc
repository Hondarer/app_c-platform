#include <testfw.h>
#include <mock_cplat.h>

uint32_t delegate_real_cplat_process_get_pid(void)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_process_get_pid)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_process_get_pid"));

    return real_fn();
}

MOCK_WEAK_IMPL(uint32_t, cplat_process_get_pid, void)
{
    uint32_t mock_ret = 0;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_process_get_pid();
    }
    else
    {
        mock_ret = delegate_real_cplat_process_get_pid();
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %u\n", mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
