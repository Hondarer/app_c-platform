#include <stdarg.h>
#include <vector>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_vstat_fmt(com_util_file_stat_t *buf, com_util_error *detail_out, const char *format,
                                     va_list args)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_vstat_fmt)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_vstat_fmt"));

    return real_fn(buf, detail_out, format, args);
}

MOCK_WEAK_IMPL(int, com_util_vstat_fmt, com_util_file_stat_t *buf, com_util_error *detail_out, const char *format,
               va_list args)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    std::vector<char> path = mock_com_util_expand_format(format, args);

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_vstat_fmt(buf, detail_out, path.data());
    }
    else
    {
        mock_ret = delegate_real_com_util_vstat_fmt(buf, detail_out, format, args);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %s", __func__, (void *)buf, path.data());
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
