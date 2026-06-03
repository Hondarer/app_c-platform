#include <testfw.h>
#include <mock_com_util.h>

com_util_tracer *delegate_real_com_util_tracer_create(void)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_tracer_create)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_tracer_create"));

    return real_fn();
}

MOCK_WEAK_IMPL(com_util_tracer *, com_util_tracer_create, void)
{
    com_util_tracer *handle = nullptr;

    if (_mock_com_util != nullptr)
    {
        handle = _mock_com_util->com_util_tracer_create();
    }
    else
    {
        handle = delegate_real_com_util_tracer_create();
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)handle);
        }
        else
        {
            printf("\n");
        }
    }

    return handle;
}
