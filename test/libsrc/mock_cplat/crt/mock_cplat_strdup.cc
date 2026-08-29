#include <testfw.h>
#include <mock_cplat.h>

char *delegate_real_cplat_strdup(const char *src)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_strdup)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_strdup"));

    return real_fn(src);
}

MOCK_WEAK_IMPL(char *, cplat_strdup, const char *src)
{
    char *mock_ret = nullptr;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_strdup(src);
    }
    else
    {
        mock_ret = delegate_real_cplat_strdup(src);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s", __func__, src);
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
