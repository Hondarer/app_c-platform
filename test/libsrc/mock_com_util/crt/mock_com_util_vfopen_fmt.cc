#include <stdarg.h>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

FILE *delegate_real_com_util_vfopen_fmt(const char *modes, int *errno_out, const char *format, va_list args)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_vfopen_fmt)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_vfopen_fmt"));

    return real_fn(modes, errno_out, format, args);
}

MOCK_WEAK_IMPL(FILE *, com_util_vfopen_fmt, const char *modes, int *errno_out, const char *format, va_list args)
{
    FILE *rtc = nullptr;

    char buf[4096];
    {
        va_list args_copy;
        va_copy(args_copy, args);
        vsnprintf(buf, sizeof(buf), format, args_copy);
        va_end(args_copy);
    }

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_vfopen_fmt(modes, errno_out, buf);
    }
    else
    {
        rtc = delegate_real_com_util_vfopen_fmt(modes, errno_out, format, args);
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
