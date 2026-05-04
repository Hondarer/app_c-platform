#include <stdarg.h>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_fprintf(FILE *stream, const char *format, ...)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_fprintf)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_fprintf"));

    return real_fn(stream, "%s", format);
}

MOCK_WEAK_IMPL(int, com_util_fprintf, FILE *stream, const char *format, ...)
{
    int rtc = -1;

    char buf[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_fprintf(stream, buf);
    }
    else
    {
        rtc = delegate_real_com_util_fprintf(stream, buf);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %s", __func__, (void *)stream, buf);
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
