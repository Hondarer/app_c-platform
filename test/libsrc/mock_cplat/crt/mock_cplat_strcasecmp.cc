#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_strcasecmp(const char *lhs, const char *rhs)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_strcasecmp)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_strcasecmp"));

    return real_fn(lhs, rhs);
}

MOCK_WEAK_IMPL(int, cplat_strcasecmp, const char *lhs, const char *rhs)
{
    int mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_strcasecmp(lhs, rhs);
    }
    else
    {
        mock_ret = delegate_real_cplat_strcasecmp(lhs, rhs);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, %s", __func__, lhs, rhs);
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
