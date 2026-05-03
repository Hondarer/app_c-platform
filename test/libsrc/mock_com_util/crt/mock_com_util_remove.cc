#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_remove(const char *path)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_remove)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_remove"));

    return real_fn(path);
}

WEAK_ATR int com_util_remove(const char *path)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_remove(path);
    }
    else
    {
        rtc = delegate_real_com_util_remove(path);
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
