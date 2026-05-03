#include <stdarg.h>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

FILE *delegate_real_com_util_fopen_fmt(const char *modes, int *errno_out, const char *format, ...)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_fopen_fmt)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_fopen_fmt"));

    return real_fn(modes, errno_out, "%s", format);
}

WEAK_ATR FILE *com_util_fopen_fmt(const char *modes, int *errno_out, const char *format, ...)
{
    FILE *rtc = nullptr;

    char buf[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_fopen_fmt(modes, errno_out, buf);
    }
    else
    {
        rtc = delegate_real_com_util_fopen_fmt(modes, errno_out, buf);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, 0x%p, %s", __func__, modes, (void *)errno_out, buf);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}
