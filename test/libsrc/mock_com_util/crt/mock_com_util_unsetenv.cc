#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_unsetenv(const char *name, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_unsetenv)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_unsetenv"));

    return real_fn(name, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_unsetenv, const char *name, com_util_error *detail_out)
{
    int rtc = EINVAL;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_unsetenv(name, detail_out);
    }
    else
    {
        rtc = delegate_real_com_util_unsetenv(name, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s", __func__, name);
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
