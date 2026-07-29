#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_module_get_path(char *out_path, size_t out_path_sz, const void *func_addr)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_module_get_path)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_module_get_path"));

    return real_fn(out_path, out_path_sz, func_addr);
}

MOCK_WEAK_IMPL(int, com_util_module_get_path, char *out_path, size_t out_path_sz, const void *func_addr)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_module_get_path(out_path, out_path_sz, func_addr);
    }
    else
    {
        rtc = delegate_real_com_util_module_get_path(out_path, out_path_sz, func_addr);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, func_addr);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}
