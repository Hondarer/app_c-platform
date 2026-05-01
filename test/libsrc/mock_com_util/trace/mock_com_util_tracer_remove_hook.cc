#include <testfw.h>
#include <mock_com_util.h>

WEAK_ATR void com_util_tracer_remove_hook(com_util_tracer_t *handle,
                                          com_util_tracer_hook_entry_t *hook_entry)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_tracer_remove_hook(handle, hook_entry);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, 0x%p\n", __func__, (void *)handle, (void *)hook_entry);
    }
}
