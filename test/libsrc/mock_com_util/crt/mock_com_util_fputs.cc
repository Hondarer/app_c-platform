#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_fputs(const char *str, FILE *stream)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_fputs)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_fputs"));

    return real_fn(str, stream);
}

MOCK_WEAK_IMPL(int, com_util_fputs, const char *str, FILE *stream)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_fputs(str, stream);
    }
    else
    {
        rtc = delegate_real_com_util_fputs(str, stream);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, 0x%p", __func__, str, (void *)stream);
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
