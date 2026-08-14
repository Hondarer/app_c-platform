#include <testfw.h>
#include <mock_com_util.h>

com_util_trace_level delegate_real_com_util_tracer_get_os_level(com_util_tracer *handle)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_tracer_get_os_level)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_tracer_get_os_level"));

    return real_fn(handle);
}

MOCK_WEAK_IMPL(com_util_trace_level, com_util_tracer_get_os_level, com_util_tracer *handle)
{
    com_util_trace_level mock_ret = COM_UTIL_TRACE_LEVEL_NONE;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_tracer_get_os_level(handle);
    }
    else
    {
        mock_ret = delegate_real_com_util_tracer_get_os_level(handle);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (void *)handle);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", (int)mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
