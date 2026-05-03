#include <testfw.h>
#include <mock_com_util.h>
#include <inttypes.h>

int delegate_real_com_util_format_realtime_iso8601_utc(char *buf, size_t buf_size, int64_t tv_sec, int32_t tv_nsec)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_format_realtime_iso8601_utc)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_format_realtime_iso8601_utc"));

    return real_fn(buf, buf_size, tv_sec, tv_nsec);
}

WEAK_ATR int com_util_format_realtime_iso8601_utc(char *buf, size_t buf_size, int64_t tv_sec, int32_t tv_nsec)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_format_realtime_iso8601_utc(buf, buf_size, tv_sec, tv_nsec);
    }
    else
    {
        rtc = delegate_real_com_util_format_realtime_iso8601_utc(buf, buf_size, tv_sec, tv_nsec);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu, %" PRId64 ", %" PRId32, __func__, (void *)buf, buf_size, tv_sec, tv_nsec);
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
