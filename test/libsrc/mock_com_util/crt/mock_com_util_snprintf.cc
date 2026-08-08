#include <stdarg.h>
#include <stdio.h>
#include <vector>
#include <testfw.h>
#include <mock_com_util.h>

// mock_com_util_vsnprintf.cc で定義する、書式を切り詰めずに展開するヘルパー
extern std::vector<char> mock_com_util_expand_format(const char *format, va_list args);

int delegate_real_com_util_snprintf(char *dest, size_t dest_size, const char *format, ...)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_snprintf)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_snprintf"));

    return real_fn(dest, dest_size, "%s", format);
}

MOCK_WEAK_IMPL(int, com_util_snprintf, char *dest, size_t dest_size, const char *format, ...)
{
    int rtc = -1;
    std::vector<char> buf;

    {
        va_list args;
        va_start(args, format);
        buf = mock_com_util_expand_format(format, args);
        va_end(args);
    }

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_snprintf(dest, dest_size, buf.data());
    }
    else
    {
        rtc = delegate_real_com_util_snprintf(dest, dest_size, buf.data());
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu, %s", __func__, (void *)dest, dest_size, buf.data());
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
