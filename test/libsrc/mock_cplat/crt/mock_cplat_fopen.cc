#include <testfw.h>
#include <mock_cplat.h>

FILE *delegate_real_cplat_fopen(const char *path, const char *modes, cplat_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_fopen)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_fopen"));

    return real_fn(path, modes, detail_out);
}

MOCK_WEAK_IMPL(FILE *, cplat_fopen, const char *path, const char *modes, cplat_error *detail_out)
{
    FILE *mock_ret = nullptr;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_fopen(path, modes, detail_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_fopen(path, modes, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, %s", __func__, path, modes);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
