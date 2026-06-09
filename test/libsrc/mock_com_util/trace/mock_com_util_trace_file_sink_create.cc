#include <testfw.h>
#include <mock_com_util.h>

com_util_trace_file_sink *delegate_real_com_util_trace_file_sink_create(const char *path, size_t max_bytes,
                                                                        int generations, int flags)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_trace_file_sink_create)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_trace_file_sink_create"));

    return real_fn(path, max_bytes, generations, flags);
}

MOCK_WEAK_IMPL(com_util_trace_file_sink *, com_util_trace_file_sink_create, const char *path, size_t max_bytes,
               int generations, int flags)
{
    com_util_trace_file_sink *rtc = nullptr;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_trace_file_sink_create(path, max_bytes, generations, flags);
    }
    else
    {
        rtc = delegate_real_com_util_trace_file_sink_create(path, max_bytes, generations, flags);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s \"%s\", %d", __func__, path != nullptr ? path : "(null)", flags);
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
