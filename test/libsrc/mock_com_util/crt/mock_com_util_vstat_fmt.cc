#include <stdarg.h>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_vstat_fmt(com_util_file_stat_t *buf, const char *format, va_list args)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_vstat_fmt)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_vstat_fmt"));

    return real_fn(buf, format, args);
}

MOCK_WEAK_IMPL(int, com_util_vstat_fmt, com_util_file_stat_t *buf, const char *format, va_list args)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    char path[4096];
    {
        va_list args_copy;
        va_copy(args_copy, args);
        vsnprintf(path, sizeof(path), format, args_copy);
        va_end(args_copy);
    }

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_vstat_fmt(buf, path);
    }
    else
    {
        rtc = delegate_real_com_util_vstat_fmt(buf, format, args);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %s", __func__, (void *)buf, path);
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
