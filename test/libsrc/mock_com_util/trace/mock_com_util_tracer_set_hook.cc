#include <testfw.h>
#include <mock_com_util.h>

com_util_tracer_hook_entry *delegate_real_com_util_tracer_set_hook(com_util_tracer *handle,
                                                                   com_util_tracer_hook_fn_t fn, void *context)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_tracer_set_hook)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_tracer_set_hook"));

    return real_fn(handle, fn, context);
}

MOCK_WEAK_IMPL(com_util_tracer_hook_entry *, com_util_tracer_set_hook, com_util_tracer *handle,
               com_util_tracer_hook_fn_t fn, void *context)
{
    com_util_tracer_hook_entry *entry = nullptr;

    if (_mock_com_util != nullptr)
    {
        entry = _mock_com_util->com_util_tracer_set_hook(handle, fn, context);
    }
    else
    {
        entry = delegate_real_com_util_tracer_set_hook(handle, fn, context);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, 0x%p, 0x%p", __func__, (void *)handle, (void *)(uintptr_t)fn, context);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)entry);
        }
        else
        {
            printf("\n");
        }
    }

    return entry;
}
