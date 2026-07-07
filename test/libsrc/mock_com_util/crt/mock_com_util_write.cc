#include <testfw.h>
#include <mock_com_util.h>

#include <inttypes.h>

int64_t delegate_real_com_util_write(int fd, const void *buf, size_t count)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_write)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_write"));

    return real_fn(fd, buf, count);
}

MOCK_WEAK_IMPL(int64_t, com_util_write, int fd, const void *buf, size_t count)
{
    int64_t rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_write(fd, buf, count);
    }
    else
    {
        rtc = delegate_real_com_util_write(fd, buf, count);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d, 0x%p, %zu", __func__, fd, buf, count);
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
