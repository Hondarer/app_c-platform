#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_dup(int fd)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_dup)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_dup"));

    return real_fn(fd);
}

MOCK_WEAK_IMPL(int, com_util_dup, int fd)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_dup(fd);
    }
    else
    {
        rtc = delegate_real_com_util_dup(fd);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d", __func__, fd);
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
