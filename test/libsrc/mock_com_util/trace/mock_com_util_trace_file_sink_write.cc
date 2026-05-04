#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_trace_file_sink_write(com_util_trace_file_sink_t *handle, int level,
                                            const com_util_realtime_timestamp_t *timestamp,
                                            const char *message)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_trace_file_sink_write)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_trace_file_sink_write"));

    return real_fn(handle, level, timestamp, message);
}

MOCK_WEAK_IMPL(int, com_util_trace_file_sink_write, com_util_trace_file_sink_t *handle, int level,
                                            const com_util_realtime_timestamp_t *timestamp,
                                            const char *message)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_trace_file_sink_write(handle, level, timestamp, message);
    }
    else
    {
        rtc = delegate_real_com_util_trace_file_sink_write(handle, level, timestamp, message);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d 0x%p \"%s\"", __func__, level, (const void *)timestamp, message != nullptr ? message : "(null)");
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
