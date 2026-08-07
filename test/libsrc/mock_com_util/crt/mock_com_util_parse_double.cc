#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_parse_double(double *value_out, const char *text)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_parse_double)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_parse_double"));

    return real_fn(value_out, text);
}

MOCK_WEAK_IMPL(int, com_util_parse_double, double *value_out, const char *text)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_parse_double(value_out, text);
    }
    else
    {
        rtc = delegate_real_com_util_parse_double(value_out, text);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %s", __func__, (void *)value_out, text);
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
