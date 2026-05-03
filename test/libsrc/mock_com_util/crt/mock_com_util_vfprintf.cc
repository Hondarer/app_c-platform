#include <stdarg.h>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_vfprintf(FILE *stream, const char *format, va_list args)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_vfprintf)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_vfprintf"));

    return real_fn(stream, format, args);
}

WEAK_ATR int com_util_vfprintf(FILE *stream, const char *format, va_list args)
{
    int rtc = -1;

    char buf[1024];
    {
        va_list args_copy;
        va_copy(args_copy, args);
        vsnprintf(buf, sizeof(buf), format, args_copy);
        va_end(args_copy);
    }

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_vfprintf(stream, buf);
    }
    else
    {
        rtc = delegate_real_com_util_vfprintf(stream, format, args);
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
