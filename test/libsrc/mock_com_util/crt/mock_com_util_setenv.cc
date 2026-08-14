#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_setenv(const char *name, const char *value, int overwrite, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_setenv)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_setenv"));

    return real_fn(name, value, overwrite, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_setenv, const char *name, const char *value, int overwrite, com_util_error *detail_out)
{
    int mock_ret = EINVAL;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_setenv(name, value, overwrite, detail_out);
    }
    else
    {
        mock_ret = delegate_real_com_util_setenv(name, value, overwrite, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, %s", __func__, name, value);
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
