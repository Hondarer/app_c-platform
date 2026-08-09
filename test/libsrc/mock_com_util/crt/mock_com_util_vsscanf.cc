#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_vsscanf(const char *buffer, const char *format, va_list args)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_vsscanf)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_vsscanf"));

    return real_fn(buffer, format, args);
}

MOCK_WEAK_IMPL(int, com_util_vsscanf, const char *buffer, const char *format, va_list args)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_vsscanf(buffer, format, args);
    }
    else
    {
        rtc = delegate_real_com_util_vsscanf(buffer, format, args);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s \"%s\", \"%s\"", __func__, buffer, format);
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
