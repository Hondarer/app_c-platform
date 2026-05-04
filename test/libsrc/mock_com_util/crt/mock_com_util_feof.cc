#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_feof(FILE *stream)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_feof)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_feof"));

    return real_fn(stream);
}

MOCK_WEAK_IMPL(int, com_util_feof, FILE *stream)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_feof(stream);
    }
    else
    {
        rtc = delegate_real_com_util_feof(stream);
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
