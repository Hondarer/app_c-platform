#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_module_get_basename(char *basename_out, size_t basename_size, const void *func_addr)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_module_get_basename)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_module_get_basename"));

    return real_fn(basename_out, basename_size, func_addr);
}

MOCK_WEAK_IMPL(int, cplat_module_get_basename, char *basename_out, size_t basename_size, const void *func_addr)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_module_get_basename(basename_out, basename_size, func_addr);
    }
    else
    {
        mock_ret = delegate_real_cplat_module_get_basename(basename_out, basename_size, func_addr);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, func_addr);
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
