#include <stdarg.h>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_stat_fmt(com_util_file_stat_t *buf, const char *format, ...)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_stat_fmt)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_stat_fmt"));

    return real_fn(buf, "%s", format);
}

WEAK_ATR int com_util_stat_fmt(com_util_file_stat_t *buf, const char *format, ...)
{
    int rtc = -1;

    char path[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(path, sizeof(path), format, args);
    va_end(args);

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_stat_fmt(buf, path);
    }
    else
    {
        rtc = delegate_real_com_util_stat_fmt(buf, path);
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
