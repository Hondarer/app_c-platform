#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_unsetenv(const char *name, cplat_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_unsetenv)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_unsetenv"));

    return real_fn(name, detail_out);
}

MOCK_WEAK_IMPL(int, cplat_unsetenv, const char *name, cplat_error *detail_out)
{
    int mock_ret = EINVAL;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_unsetenv(name, detail_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_unsetenv(name, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s", __func__, name);
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
