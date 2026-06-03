#include <testfw.h>
#include <mock_com_util.h>

com_util_trace_file_sink *delegate_real_com_util_trace_file_sink_create(const char *path, size_t max_bytes,
                                                                        int generations)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_trace_file_sink_create)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_trace_file_sink_create"));

    return real_fn(path, max_bytes, generations);
}

MOCK_WEAK_IMPL(com_util_trace_file_sink *, com_util_trace_file_sink_create, const char *path, size_t max_bytes,
               int generations)
{
    com_util_trace_file_sink *rtc = nullptr;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_trace_file_sink_create(path, max_bytes, generations);
    }
    else
    {
        rtc = delegate_real_com_util_trace_file_sink_create(path, max_bytes, generations);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s \"%s\"", __func__, path != nullptr ? path : "(null)");
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}
