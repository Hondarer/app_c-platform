#include <testfw.h>
#include <mock_com_util.h>

com_util_tracer_state delegate_real_com_util_tracer_get_state(com_util_tracer *handle)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_tracer_get_state)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_tracer_get_state"));

    return real_fn(handle);
}

MOCK_WEAK_IMPL(com_util_tracer_state, com_util_tracer_get_state, com_util_tracer *handle)
{
    com_util_tracer_state mock_ret = COM_UTIL_TRACER_STATE_DISPOSED;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_tracer_get_state(handle);
    }
    else
    {
        mock_ret = delegate_real_com_util_tracer_get_state(handle);
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
