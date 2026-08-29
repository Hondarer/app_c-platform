#include <testfw.h>
#include <mock_cplat.h>

char *delegate_real_cplat_normalize_path_sep(char *path)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_normalize_path_sep)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_normalize_path_sep"));

    return real_fn(path);
}

MOCK_WEAK_IMPL(char *, cplat_normalize_path_sep, char *path)
{
    char *mock_ret = nullptr;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_normalize_path_sep(path);
    }
    else
    {
        mock_ret = delegate_real_cplat_normalize_path_sep(path);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s", __func__, (path != nullptr) ? path : "(null)");
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %s\n", (mock_ret != nullptr) ? mock_ret : "(null)");
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
