#include <stdarg.h>
#include <vector>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

FILE *delegate_real_com_util_fopen_fmt(const char *modes, com_util_error *detail_out, const char *format, ...)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_fopen_fmt)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_fopen_fmt"));

    return real_fn(modes, detail_out, "%s", format);
}

MOCK_WEAK_IMPL(FILE *, com_util_fopen_fmt, const char *modes, com_util_error *detail_out, const char *format, ...)
{
    FILE *rtc = nullptr;

    std::vector<char> buf;
    {
        va_list args;
        va_start(args, format);
        buf = mock_com_util_expand_format(format, args);
        va_end(args);
    }

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_fopen_fmt(modes, detail_out, buf.data());
    }
    else
    {
        rtc = delegate_real_com_util_fopen_fmt(modes, detail_out, buf.data());
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, 0x%p, %s", __func__, modes, (void *)detail_out, buf.data());
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
