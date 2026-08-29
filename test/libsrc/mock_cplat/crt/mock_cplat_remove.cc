#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_remove(const char *path, cplat_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_remove)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_remove"));

    return real_fn(path, detail_out);
}

MOCK_WEAK_IMPL(int, cplat_remove, const char *path, cplat_error *detail_out)
{
    int mock_ret = -1;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_remove(path, detail_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_remove(path, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s", __func__, path);
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
