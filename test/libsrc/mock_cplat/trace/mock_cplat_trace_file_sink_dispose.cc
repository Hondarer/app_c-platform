#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_trace_file_sink_dispose(cplat_trace_file_sink *handle)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_trace_file_sink_dispose)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_trace_file_sink_dispose"));

    real_fn(handle);
}

MOCK_WEAK_IMPL(void, cplat_trace_file_sink_dispose, cplat_trace_file_sink *handle)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_trace_file_sink_dispose(handle);
    }
    else
    {
        delegate_real_cplat_trace_file_sink_dispose(handle);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p\n", __func__, (void *)handle);
    }
}
