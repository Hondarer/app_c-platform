#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_parse_int(int *value_out, const char *text, int base)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_parse_int)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_parse_int"));

    return real_fn(value_out, text, base);
}

MOCK_WEAK_IMPL(int, com_util_parse_int, int *value_out, const char *text, int base)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_parse_int(value_out, text, base);
    }
    else
    {
        rtc = delegate_real_com_util_parse_int(value_out, text, base);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %s, %d", __func__, (void *)value_out, text, base);
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
