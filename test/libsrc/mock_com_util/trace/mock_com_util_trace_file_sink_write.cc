#include <testfw.h>
#include <mock_com_util.h>

WEAK_ATR int com_util_trace_file_sink_write(com_util_trace_file_sink_t *handle, int level,
                                            const com_util_realtime_timestamp_t *timestamp,
                                            const char *message)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_trace_file_sink_write(handle, level, timestamp, message);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d 0x%p \"%s\"", __func__, level, (void *)timestamp, message != nullptr ? message : "(null)");
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
