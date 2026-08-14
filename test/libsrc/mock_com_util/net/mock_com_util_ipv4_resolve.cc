#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_ipv4_resolve(const char *text, uint32_t *address_out, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_ipv4_resolve)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_ipv4_resolve"));

    return real_fn(text, address_out, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_ipv4_resolve, const char *text, uint32_t *address_out, com_util_error *detail_out)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_ipv4_resolve(text, address_out, detail_out);
    }
    else
    {
        mock_ret = delegate_real_com_util_ipv4_resolve(text, address_out, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
