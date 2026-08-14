#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_module_get_basename(char *out_basename, size_t out_basename_sz, const void *func_addr)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_module_get_basename)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_module_get_basename"));

    return real_fn(out_basename, out_basename_sz, func_addr);
}

MOCK_WEAK_IMPL(int, com_util_module_get_basename, char *out_basename, size_t out_basename_sz, const void *func_addr)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_module_get_basename(out_basename, out_basename_sz, func_addr);
    }
    else
    {
        mock_ret = delegate_real_com_util_module_get_basename(out_basename, out_basename_sz, func_addr);
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
