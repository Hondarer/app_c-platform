#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_tracer_write_with_source(com_util_tracer *handle, com_util_trace_level level,
                                                    const com_util_timespec *timestamp, const char *file, int line,
                                                    const char *message)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_tracer_write_with_source)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_tracer_write_with_source"));

    return real_fn(handle, level, timestamp, file, line, message);
}

MOCK_WEAK_IMPL(int, com_util_tracer_write_with_source, com_util_tracer *handle, com_util_trace_level level,
               const com_util_timespec *timestamp, const char *file, int line, const char *message)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_tracer_write_with_source(handle, level, timestamp, file, line, message);
    }
    else
    {
        rtc = delegate_real_com_util_tracer_write_with_source(handle, level, timestamp, file, line, message);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %d, %s:%d, %s -> %d\n", __func__, (void *)handle, (int)level,
               file == NULL ? "(null)" : file, line, message == NULL ? "(null)" : message, rtc);
    }

    return rtc;
}
