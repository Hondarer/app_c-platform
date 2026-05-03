#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_fflush(FILE *stream)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_fflush)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_fflush"));

    return real_fn(stream);
}

WEAK_ATR int com_util_fflush(FILE *stream)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_fflush(stream);
    }
    else
    {
        rtc = delegate_real_com_util_fflush(stream);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (void *)stream);
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
