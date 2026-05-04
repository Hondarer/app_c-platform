#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_getenv(const char *name, char *buf, size_t buf_size)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_getenv)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_getenv"));

    return real_fn(name, buf, buf_size);
}

MOCK_WEAK_IMPL(int, com_util_getenv, const char *name, char *buf, size_t buf_size)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_getenv(name, buf, buf_size);
    }
    else
    {
        rtc = delegate_real_com_util_getenv(name, buf, buf_size);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, 0x%p, %zu", __func__, name, (void *)buf, buf_size);
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
