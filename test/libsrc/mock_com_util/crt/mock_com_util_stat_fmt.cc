#include <stdarg.h>
#include <vector>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_stat_fmt(com_util_file_stat_t *buf, com_util_error *detail_out, const char *format, ...)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_stat_fmt)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_stat_fmt"));

    return real_fn(buf, detail_out, "%s", format);
}

MOCK_WEAK_IMPL(int, com_util_stat_fmt, com_util_file_stat_t *buf, com_util_error *detail_out, const char *format, ...)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    std::vector<char> path;
    {
        va_list args;
        va_start(args, format);
        path = mock_com_util_expand_format(format, args);
        va_end(args);
    }

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_stat_fmt(buf, detail_out, path.data());
    }
    else
    {
        rtc = delegate_real_com_util_stat_fmt(buf, detail_out, path.data());
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %s", __func__, (void *)buf, path.data());
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
