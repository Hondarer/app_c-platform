#include <stdarg.h>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_access_fmt(int mode, const char *format, ...)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_access_fmt)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_access_fmt"));

    return real_fn(mode, "%s", format);
}

WEAK_ATR int com_util_access_fmt(int mode, const char *format, ...)
{
    int rtc = -1;

    char buf[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_access_fmt(mode, buf);
    }
    else
    {
        rtc = delegate_real_com_util_access_fmt(mode, buf);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d, %s", __func__, mode, buf);
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
