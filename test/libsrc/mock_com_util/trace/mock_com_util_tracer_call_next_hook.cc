#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_tracer_call_next_hook(com_util_tracer_hook_entry_t *prev,
                                              com_util_tracer_t *handle,
                                              com_util_trace_level_t level,
                                              const com_util_realtime_timestamp_t *timestamp,
                                              const char *message)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_tracer_call_next_hook)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_tracer_call_next_hook"));

    real_fn(prev, handle, level, timestamp, message);
}

WEAK_ATR void com_util_tracer_call_next_hook(com_util_tracer_hook_entry_t *prev,
                                              com_util_tracer_t *handle,
                                              com_util_trace_level_t level,
                                              const com_util_realtime_timestamp_t *timestamp,
                                              const char *message)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_tracer_call_next_hook(prev, handle, level, timestamp, message);
    }
    else
    {
        delegate_real_com_util_tracer_call_next_hook(prev, handle, level, timestamp, message);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, 0x%p, %d, 0x%p, %s\n", __func__,
               (void *)prev, (void *)handle, (int)level, (const void *)timestamp, message);
    }
}
