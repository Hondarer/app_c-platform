#include <stdarg.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_path_join_n(char *path_out, size_t path_size, com_util_error *detail_out, size_t part_count,
                                       ...)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_vpath_join_n)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_vpath_join_n"));
    va_list args;
    int rtc;

    va_start(args, part_count);
    rtc = real_fn(path_out, path_size, detail_out, part_count, args);
    va_end(args);
    return rtc;
}

MOCK_WEAK_IMPL(int, com_util_path_join_n, char *path_out, size_t path_size, com_util_error *detail_out,
               size_t part_count, ...)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;
    va_list args;

    va_start(args, part_count);
    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_path_join_n(path_out, path_size, detail_out, part_count, args);
    }
    else
    {
        rtc = delegate_real_com_util_vpath_join_n(path_out, path_size, detail_out, part_count, args);
    }
    va_end(args);

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %zu", __func__, part_count);
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
