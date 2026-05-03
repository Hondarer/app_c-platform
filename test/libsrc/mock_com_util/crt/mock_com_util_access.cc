#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_access(const char *path, int mode)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_access)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_access"));

    return real_fn(path, mode);
}

WEAK_ATR int com_util_access(const char *path, int mode)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_access(path, mode);
    }
    else
    {
        rtc = delegate_real_com_util_access(path, mode);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, %d", __func__, path, mode);
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
