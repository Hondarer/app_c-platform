#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_parse_int64(int64_t *value_out, const char *text, int base)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_parse_int64)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_parse_int64"));

    return real_fn(value_out, text, base);
}

MOCK_WEAK_IMPL(int, com_util_parse_int64, int64_t *value_out, const char *text, int base)
{
    int mock_ret = -1;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_parse_int64(value_out, text, base);
    }
    else
    {
        mock_ret = delegate_real_com_util_parse_int64(value_out, text, base);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %s, %d", __func__, (void *)value_out, text, base);
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
