#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_rename(const char *oldpath, const char *newpath, cplat_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_rename)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_rename"));

    return real_fn(oldpath, newpath, detail_out);
}

MOCK_WEAK_IMPL(int, cplat_rename, const char *oldpath, const char *newpath, cplat_error *detail_out)
{
    int mock_ret = -1;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_rename(oldpath, newpath, detail_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_rename(oldpath, newpath, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, %s", __func__, oldpath, newpath);
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
