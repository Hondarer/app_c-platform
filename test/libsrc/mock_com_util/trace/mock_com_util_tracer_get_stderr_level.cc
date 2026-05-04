#include <testfw.h>
#include <mock_com_util.h>

com_util_trace_level_t delegate_real_com_util_tracer_get_stderr_level(com_util_tracer_t *handle)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_tracer_get_stderr_level)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_tracer_get_stderr_level"));

    return real_fn(handle);
}

MOCK_WEAK_IMPL(com_util_trace_level_t, com_util_tracer_get_stderr_level, com_util_tracer_t *handle)
{
    com_util_trace_level_t rtc = COM_UTIL_TRACE_LEVEL_NONE;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_tracer_get_stderr_level(handle);
    }
    else
    {
        rtc = delegate_real_com_util_tracer_get_stderr_level(handle);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (void *)handle);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", (int)rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}
