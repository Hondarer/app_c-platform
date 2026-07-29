#include <testfw.h>
#include <mock_com_util.h>
#include <inttypes.h>

int delegate_real_com_util_format_realtime_iso8601_local(char *buf, size_t buf_size, const com_util_timespec *timestamp)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_format_realtime_iso8601_local)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_format_realtime_iso8601_local"));

    return real_fn(buf, buf_size, timestamp);
}

MOCK_WEAK_IMPL(int, com_util_format_realtime_iso8601_local, char *buf, size_t buf_size,
               const com_util_timespec *timestamp)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_format_realtime_iso8601_local(buf, buf_size, timestamp);
    }
    else
    {
        rtc = delegate_real_com_util_format_realtime_iso8601_local(buf, buf_size, timestamp);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu, 0x%p", __func__, (void *)buf, buf_size, (const void *)timestamp);
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
