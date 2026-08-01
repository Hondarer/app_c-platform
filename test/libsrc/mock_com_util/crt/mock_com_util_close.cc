#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_close(int fd, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_close)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_close"));

    return real_fn(fd, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_close, int fd, com_util_error *detail_out)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_close(fd, detail_out);
    }
    else
    {
        rtc = delegate_real_com_util_close(fd, detail_out);
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
