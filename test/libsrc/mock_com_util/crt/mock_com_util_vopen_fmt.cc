#include <stdarg.h>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_vopen_fmt(int flags, int mode, com_util_error *detail_out, const char *format, va_list args)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_vopen_fmt)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_vopen_fmt"));

    return real_fn(flags, mode, detail_out, format, args);
}

MOCK_WEAK_IMPL(int, com_util_vopen_fmt, int flags, int mode, com_util_error *detail_out, const char *format,
               va_list args)
{
    int rtc = -1;

    char buf[4096];
    {
        va_list args_copy;
        va_copy(args_copy, args);
        vsnprintf(buf, sizeof(buf), format, args_copy);
        va_end(args_copy);
    }

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_vopen_fmt(flags, mode, detail_out, buf);
    }
    else
    {
        rtc = delegate_real_com_util_vopen_fmt(flags, mode, detail_out, format, args);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d, %d, %s", __func__, flags, mode, buf);
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
