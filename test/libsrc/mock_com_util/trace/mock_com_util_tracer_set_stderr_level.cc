#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_tracer_set_stderr_level(com_util_tracer_t *handle, com_util_trace_level_t level)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_tracer_set_stderr_level)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_tracer_set_stderr_level"));

    return real_fn(handle, level);
}

WEAK_ATR int com_util_tracer_set_stderr_level(com_util_tracer_t *handle, com_util_trace_level_t level)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_tracer_set_stderr_level(handle, level);
    }
    else
    {
        rtc = delegate_real_com_util_tracer_set_stderr_level(handle, level);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %d", __func__, (void *)handle, (int)level);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}
