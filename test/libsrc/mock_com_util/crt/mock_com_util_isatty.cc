#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_isatty(com_util_stream stream)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_isatty)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_isatty"));

    return real_fn(stream);
}

MOCK_WEAK_IMPL(int, com_util_isatty, com_util_stream stream)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_isatty(stream);
    }
    else
    {
        rtc = delegate_real_com_util_isatty(stream);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d", __func__, (int)stream);
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
