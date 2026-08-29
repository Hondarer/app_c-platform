#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_host_get_name(char *name_out, size_t name_size)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_host_get_name)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_host_get_name"));

    return real_fn(name_out, name_size);
}

MOCK_WEAK_IMPL(int, cplat_host_get_name, char *name_out, size_t name_size)
{
    int mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_host_get_name(name_out, name_size);
    }
    else
    {
        mock_ret = delegate_real_cplat_host_get_name(name_out, name_size);
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
