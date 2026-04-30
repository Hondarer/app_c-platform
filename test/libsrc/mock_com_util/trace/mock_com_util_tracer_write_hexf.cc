#include <stdarg.h>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

WEAK_ATR int com_util_tracer_write_hexf(com_util_tracer_t *handle, com_util_trace_level_t level,
                                        const com_util_realtime_timestamp_t *timestamp,
                                        const void *data, size_t size,
                                        const char *format, ...)
{
    int rtc = 0;

    char label[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(label, sizeof(label), format, args);
    va_end(args);

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_tracer_write_hexf(
            handle, level, timestamp, data, size, label);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %d, 0x%p, 0x%p, %zu, %s", __func__,
               (void *)handle, (int)level, (void *)timestamp, data, size, label);
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
