#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_tracer_set_file_level(com_util_tracer *handle, const char *path, com_util_trace_level level,
                                                 size_t max_bytes, int generations, int flags)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_tracer_set_file_level)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_tracer_set_file_level"));

    return real_fn(handle, path, level, max_bytes, generations, flags);
}

MOCK_WEAK_IMPL(int, com_util_tracer_set_file_level, com_util_tracer *handle, const char *path,
               com_util_trace_level level, size_t max_bytes, int generations, int flags)
{
    int mock_ret = 0;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_tracer_set_file_level(handle, path, level, max_bytes, generations, flags);
    }
    else
    {
        mock_ret = delegate_real_com_util_tracer_set_file_level(handle, path, level, max_bytes, generations, flags);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %s, %d, %zu, %d, %d", __func__, (void *)handle, path, (int)level, max_bytes, generations,
               flags);
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
