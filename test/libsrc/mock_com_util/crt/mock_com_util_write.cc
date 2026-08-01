#include <testfw.h>
#include <mock_com_util.h>

#include <inttypes.h>

int64_t delegate_real_com_util_write(int fd, const void *buf, size_t count, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_write)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_write"));

    return real_fn(fd, buf, count, detail_out);
}

MOCK_WEAK_IMPL(int64_t, com_util_write, int fd, const void *buf, size_t count, com_util_error *detail_out)
{
    int64_t rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_write(fd, buf, count, detail_out);
    }
    else
    {
        rtc = delegate_real_com_util_write(fd, buf, count, detail_out);
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
