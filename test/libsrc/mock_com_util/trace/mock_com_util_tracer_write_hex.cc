#include <testfw.h>
#include <mock_com_util.h>

int delegate_real__com_util_tracer_write_hex(com_util_tracer *handle, com_util_trace_level_t level,
                                             const com_util_timespec *timestamp, const void *data, size_t size,
                                             const char *message)
{
    static auto real_fn = reinterpret_cast<decltype(&_com_util_tracer_write_hex)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "_com_util_tracer_write_hex"));

    return real_fn(handle, level, timestamp, data, size, message);
}

MOCK_WEAK_IMPL(int, _com_util_tracer_write_hex, com_util_tracer *handle, com_util_trace_level_t level,
               const com_util_timespec *timestamp, const void *data, size_t size, const char *message)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->_com_util_tracer_write_hex(handle, level, timestamp, data, size, message);
    }
    else
    {
        rtc = delegate_real__com_util_tracer_write_hex(handle, level, timestamp, data, size, message);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %d, 0x%p, 0x%p, %zu, %s", __func__, (void *)handle, (int)level, (const void *)timestamp,
               data, size, message);
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
