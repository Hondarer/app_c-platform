#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_ctime(char *buf, size_t buf_size, const time_t *timep)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_ctime)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_ctime"));

    return real_fn(buf, buf_size, timep);
}

MOCK_WEAK_IMPL(int, cplat_ctime, char *buf, size_t buf_size, const time_t *timep)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_ctime(buf, buf_size, timep);
    }
    else
    {
        mock_ret = delegate_real_cplat_ctime(buf, buf_size, timep);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu, 0x%p", __func__, (void *)buf, buf_size, (const void *)timep);
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
