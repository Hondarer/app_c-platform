#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_get_hostname(char *name_out, size_t name_size)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_get_hostname)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_get_hostname"));

    return real_fn(name_out, name_size);
}

MOCK_WEAK_IMPL(int, cplat_get_hostname, char *name_out, size_t name_size)
{
    int mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_get_hostname(name_out, name_size);
    }
    else
    {
        mock_ret = delegate_real_cplat_get_hostname(name_out, name_size);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu", __func__, (void *)name_out, name_size);
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
