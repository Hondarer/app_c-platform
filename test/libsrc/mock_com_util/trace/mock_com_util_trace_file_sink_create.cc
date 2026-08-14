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
    com_util_trace_file_sink *mock_ret = nullptr;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_trace_file_sink_create(path, max_bytes, generations, flags);
    }
    else
    {
        mock_ret = delegate_real_com_util_trace_file_sink_create(path, max_bytes, generations, flags);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s \"%s\", %d", __func__, path != nullptr ? path : "(null)", flags);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
