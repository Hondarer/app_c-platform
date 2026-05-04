#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_trace_file_sink_dispose(com_util_trace_file_sink_t *handle)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_trace_file_sink_dispose)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_trace_file_sink_dispose"));

    real_fn(handle);
}

MOCK_WEAK_IMPL(void, com_util_trace_file_sink_dispose, com_util_trace_file_sink_t *handle)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_trace_file_sink_dispose(handle);
    }
    else
    {
        delegate_real_com_util_trace_file_sink_dispose(handle);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p\n", __func__, (void *)handle);
    }
}
