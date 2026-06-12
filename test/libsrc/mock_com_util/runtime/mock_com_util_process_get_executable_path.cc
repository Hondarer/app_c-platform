#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_process_get_executable_path(char *out_path, size_t out_path_sz)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_process_get_executable_path)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_process_get_executable_path"));

    return real_fn(out_path, out_path_sz);
}

MOCK_WEAK_IMPL(int, com_util_process_get_executable_path, char *out_path, size_t out_path_sz)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_process_get_executable_path(out_path, out_path_sz);
    }
    else
    {
        rtc = delegate_real_com_util_process_get_executable_path(out_path, out_path_sz);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
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
