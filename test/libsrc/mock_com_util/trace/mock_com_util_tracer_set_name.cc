#include <inttypes.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_tracer_set_name(com_util_tracer *handle, const char *name, int64_t identifier)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_tracer_set_name)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_tracer_set_name"));

    return real_fn(handle, name, identifier);
}

MOCK_WEAK_IMPL(int, com_util_tracer_set_name, com_util_tracer *handle, const char *name, int64_t identifier)
{
    int mock_ret = 0;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_tracer_set_name(handle, name, identifier);
    }
    else
    {
        mock_ret = delegate_real_com_util_tracer_set_name(handle, name, identifier);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %s, %" PRId64, __func__, (void *)handle, name, identifier);
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
