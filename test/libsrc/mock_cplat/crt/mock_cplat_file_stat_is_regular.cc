#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_file_stat_is_regular(const cplat_file_stat_t *file_stat)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_file_stat_is_regular)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_file_stat_is_regular"));

    return real_fn(file_stat);
}

MOCK_WEAK_IMPL(int, cplat_file_stat_is_regular, const cplat_file_stat_t *file_stat)
{
    int mock_ret = 0;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_file_stat_is_regular(file_stat);
    }
    else
    {
        mock_ret = delegate_real_cplat_file_stat_is_regular(file_stat);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (const void *)file_stat);
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
