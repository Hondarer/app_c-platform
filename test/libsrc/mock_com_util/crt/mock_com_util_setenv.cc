#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_setenv(const char *name, const char *value, int overwrite)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_setenv)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_setenv"));

    return real_fn(name, value, overwrite);
}

MOCK_WEAK_IMPL(int, com_util_setenv, const char *name, const char *value, int overwrite)
{
    int rtc = EINVAL;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_setenv(name, value, overwrite);
    }
    else
    {
        rtc = delegate_real_com_util_setenv(name, value, overwrite);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, %s", __func__, name, value);
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
