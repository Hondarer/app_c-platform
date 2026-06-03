#include <testfw.h>
#include <mock_com_util.h>

int delegate_real__com_util_tracer_write(com_util_tracer *handle, com_util_trace_level_t level,
                                         const com_util_realtime_timestamp *timestamp, const char *message)
{
    static auto real_fn = reinterpret_cast<decltype(&_com_util_tracer_write)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "_com_util_tracer_write"));

    return real_fn(handle, level, timestamp, message);
}

MOCK_WEAK_IMPL(int, _com_util_tracer_write, com_util_tracer *handle, com_util_trace_level_t level,
               const com_util_realtime_timestamp *timestamp, const char *message)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->_com_util_tracer_write(handle, level, timestamp, message);
    }
    else
    {
        rtc = delegate_real__com_util_tracer_write(handle, level, timestamp, message);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %d, 0x%p, %s", __func__, (void *)handle, (int)level, (const void *)timestamp, message);
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
