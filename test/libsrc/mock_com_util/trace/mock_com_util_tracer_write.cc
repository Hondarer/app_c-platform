#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_tracer_write_at(com_util_tracer *handle, com_util_trace_level level,
                                         const com_util_timespec *timestamp, const char *message)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_tracer_write_at)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_tracer_write_at"));

    return real_fn(handle, level, timestamp, message);
}

MOCK_WEAK_IMPL(int, com_util_tracer_write_at, com_util_tracer *handle, com_util_trace_level level,
               const com_util_timespec *timestamp, const char *message)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_tracer_write_at(handle, level, timestamp, message);
    }
    else
    {
        rtc = delegate_real_com_util_tracer_write_at(handle, level, timestamp, message);
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
