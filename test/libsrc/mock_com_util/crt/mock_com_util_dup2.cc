#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_dup2(int oldfd, int newfd)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_dup2)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_dup2"));

    return real_fn(oldfd, newfd);
}

MOCK_WEAK_IMPL(int, com_util_dup2, int oldfd, int newfd)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_dup2(oldfd, newfd);
    }
    else
    {
        rtc = delegate_real_com_util_dup2(oldfd, newfd);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d, %d", __func__, oldfd, newfd);
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
