#include <stdarg.h>
#include <vector>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

FILE *delegate_real_com_util_vfopen_fmt(const char *modes, com_util_error *detail_out, const char *format, va_list args)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_vfopen_fmt)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_vfopen_fmt"));

    return real_fn(modes, detail_out, format, args);
}

MOCK_WEAK_IMPL(FILE *, com_util_vfopen_fmt, const char *modes, com_util_error *detail_out, const char *format,
               va_list args)
{
    FILE *rtc = nullptr;

    std::vector<char> buf = mock_com_util_expand_format(format, args);

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_vfopen_fmt(modes, detail_out, buf.data());
    }
    else
    {
        rtc = delegate_real_com_util_vfopen_fmt(modes, detail_out, format, args);
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
