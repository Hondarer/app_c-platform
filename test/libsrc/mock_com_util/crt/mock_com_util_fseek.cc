#include <stdint.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_fseek(FILE *stream, int64_t offset, int whence)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_fseek)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_fseek"));

    return real_fn(stream, offset, whence);
}

WEAK_ATR int com_util_fseek(FILE *stream, int64_t offset, int whence)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_fseek(stream, offset, whence);
    }
    else
    {
        rtc = delegate_real_com_util_fseek(stream, offset, whence);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %" PRId64 ", %d", __func__, (void *)stream, offset, whence);
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
