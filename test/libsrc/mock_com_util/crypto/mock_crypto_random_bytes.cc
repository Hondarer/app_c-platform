#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_random_bytes(void *buf, size_t size)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_random_bytes)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_random_bytes"));

    return real_fn(buf, size);
}

MOCK_WEAK_IMPL(int, com_util_random_bytes, void *buf, size_t size)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_random_bytes(buf, size);
    }
    else
    {
        rtc = delegate_real_com_util_random_bytes(buf, size);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s size=%zu", __func__, size);
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
