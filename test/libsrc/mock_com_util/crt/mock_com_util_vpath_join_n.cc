#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_vpath_join_n(char *path_out, size_t path_size, com_util_error *detail_out, size_t part_count,
                                        va_list args)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_vpath_join_n)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_vpath_join_n"));

    return real_fn(path_out, path_size, detail_out, part_count, args);
}

MOCK_WEAK_IMPL(int, com_util_vpath_join_n, char *path_out, size_t path_size, com_util_error *detail_out,
               size_t part_count, va_list args)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_vpath_join_n(path_out, path_size, detail_out, part_count, args);
    }
    else
    {
        mock_ret = delegate_real_com_util_vpath_join_n(path_out, path_size, detail_out, part_count, args);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %zu", __func__, part_count);
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
