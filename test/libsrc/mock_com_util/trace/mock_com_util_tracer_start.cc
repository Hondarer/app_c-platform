#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_tracer_start(com_util_tracer *handle)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_tracer_start)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_tracer_start"));

    return real_fn(handle);
}

MOCK_WEAK_IMPL(int, com_util_tracer_start, com_util_tracer *handle)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_tracer_start(handle);
    }
    else
    {
        rtc = delegate_real_com_util_tracer_start(handle);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (void *)handle);
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
