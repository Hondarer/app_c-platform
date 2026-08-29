#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_open(const char *path, int flags, int mode, cplat_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_open)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_open"));

    return real_fn(path, flags, mode, detail_out);
}

MOCK_WEAK_IMPL(int, cplat_open, const char *path, int flags, int mode, cplat_error *detail_out)
{
    int mock_ret = -1;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_open(path, flags, mode, detail_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_open(path, flags, mode, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, %d, %d", __func__, path, flags, mode);
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
