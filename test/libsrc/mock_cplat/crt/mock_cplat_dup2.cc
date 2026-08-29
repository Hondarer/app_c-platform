#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_dup2(int oldfd, int newfd, cplat_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_dup2)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_dup2"));

    return real_fn(oldfd, newfd, detail_out);
}

MOCK_WEAK_IMPL(int, cplat_dup2, int oldfd, int newfd, cplat_error *detail_out)
{
    int mock_ret = -1;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_dup2(oldfd, newfd, detail_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_dup2(oldfd, newfd, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d, %d", __func__, oldfd, newfd);
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
