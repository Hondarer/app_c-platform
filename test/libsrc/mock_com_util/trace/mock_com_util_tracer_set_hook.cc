#include <testfw.h>
#include <mock_com_util.h>

WEAK_ATR com_util_tracer_hook_entry_t *com_util_tracer_set_hook(
    com_util_tracer_t *handle, com_util_tracer_hook_fn_t fn, void *context)
{
    com_util_tracer_hook_entry_t *entry = nullptr;

    if (_mock_com_util != nullptr)
    {
        entry = _mock_com_util->com_util_tracer_set_hook(handle, fn, context);
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
