#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/console/console_internal.h>

void delegate_real_com_util_console_dispose_on_shutdown(const com_util_shutdown_event_t *event, void *context)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_console_dispose_on_shutdown)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_console_dispose_on_shutdown"));

    real_fn(event, context);
}

MOCK_WEAK_IMPL(void, com_util_console_dispose_on_shutdown, const com_util_shutdown_event_t *event, void *context)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_console_dispose_on_shutdown(event, context);
    }
    else
    {
        delegate_real_com_util_console_dispose_on_shutdown(event, context);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
