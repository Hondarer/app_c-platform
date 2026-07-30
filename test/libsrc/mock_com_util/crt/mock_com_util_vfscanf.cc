#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_vfscanf(FILE *stream, const char *format, va_list args)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_vfscanf)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_vfscanf"));

    return real_fn(stream, format, args);
}

MOCK_WEAK_IMPL(int, com_util_vfscanf, FILE *stream, const char *format, va_list args)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_vfscanf(stream, format, args);
    }
    else
    {
        rtc = delegate_real_com_util_vfscanf(stream, format, args);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, \"%s\"", __func__, (void *)stream, format);
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
