#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_tracer_write_hex_at(com_util_tracer *handle, com_util_trace_level level,
                                             const com_util_timespec *timestamp, const void *data, size_t size,
                                             const char *message)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_tracer_write_hex_at)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_tracer_write_hex_at"));

    return real_fn(handle, level, timestamp, data, size, message);
}

MOCK_WEAK_IMPL(int, com_util_tracer_write_hex_at, com_util_tracer *handle, com_util_trace_level level,
               const com_util_timespec *timestamp, const void *data, size_t size, const char *message)
{
    int mock_ret = 0;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_tracer_write_hex_at(handle, level, timestamp, data, size, message);
    }
    else
    {
        mock_ret = delegate_real_com_util_tracer_write_hex_at(handle, level, timestamp, data, size, message);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %d, 0x%p, 0x%p, %zu, %s", __func__, (void *)handle, (int)level, (const void *)timestamp,
               data, size, message);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
