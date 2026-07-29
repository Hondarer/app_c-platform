#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_makedirs(const char *path)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_makedirs)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_makedirs"));

    return real_fn(path);
}

MOCK_WEAK_IMPL(int, com_util_makedirs, const char *path)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_makedirs(path);
    }
    else
    {
        rtc = delegate_real_com_util_makedirs(path);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s", __func__, path);
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
