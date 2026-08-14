#include <testfw.h>
#include <mock_com_util.h>

com_util_tracer *delegate_real_com_util_tracer_create(const com_util_tracer_concurrency_mode concurrency_mode)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_tracer_create)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_tracer_create"));

    return real_fn(concurrency_mode);
}

MOCK_WEAK_IMPL(com_util_tracer *, com_util_tracer_create, com_util_tracer_concurrency_mode concurrency_mode)
{
    com_util_tracer *mock_ret = nullptr;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_tracer_create(concurrency_mode);
    }
    else
    {
        mock_ret = delegate_real_com_util_tracer_create(concurrency_mode);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
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
