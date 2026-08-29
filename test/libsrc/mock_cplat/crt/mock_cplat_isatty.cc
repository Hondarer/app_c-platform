#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_isatty(cplat_stream stream)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_isatty)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_isatty"));

    return real_fn(stream);
}

MOCK_WEAK_IMPL(int, cplat_isatty, cplat_stream stream)
{
    int mock_ret = 0;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_isatty(stream);
    }
    else
    {
        mock_ret = delegate_real_cplat_isatty(stream);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d", __func__, (int)stream);
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
