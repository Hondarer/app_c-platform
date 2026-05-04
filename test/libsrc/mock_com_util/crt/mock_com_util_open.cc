#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_open(const char *path, int flags, int mode)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_open)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_open"));

    return real_fn(path, flags, mode);
}

MOCK_WEAK_IMPL(int, com_util_open, const char *path, int flags, int mode)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_open(path, flags, mode);
    }
    else
    {
        rtc = delegate_real_com_util_open(path, flags, mode);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, %d, %d", __func__, path, flags, mode);
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
