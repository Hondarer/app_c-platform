#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_ipv4_parse(const char *text, uint32_t *address_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_ipv4_parse)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_ipv4_parse"));

    return real_fn(text, address_out);
}

MOCK_WEAK_IMPL(int, com_util_ipv4_parse, const char *text, uint32_t *address_out)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_ipv4_parse(text, address_out);
    }
    else
    {
        rtc = delegate_real_com_util_ipv4_parse(text, address_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
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
