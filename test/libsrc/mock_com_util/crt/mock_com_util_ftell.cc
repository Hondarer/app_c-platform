#include <stdint.h>
#include <testfw.h>
#include <mock_com_util.h>

int64_t delegate_real_com_util_ftell(FILE *stream)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_ftell)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_ftell"));

    return real_fn(stream);
}

MOCK_WEAK_IMPL(int64_t, com_util_ftell, FILE *stream)
{
    int64_t rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_ftell(stream);
    }
    else
    {
        rtc = delegate_real_com_util_ftell(stream);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (void *)stream);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %" PRId64 "\n", rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}
