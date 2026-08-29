#include <testfw.h>
#include <mock_cplat.h>
#include <cplat/console/console_internal.h>

void delegate_real_cplat_console_dispose_on_shutdown(const cplat_shutdown_event *event, void *context)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_console_dispose_on_shutdown)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_console_dispose_on_shutdown"));

    real_fn(event, context);
}

MOCK_WEAK_IMPL(void, cplat_console_dispose_on_shutdown, const cplat_shutdown_event *event, void *context)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_console_dispose_on_shutdown(event, context);
    }
    else
    {
        delegate_real_cplat_console_dispose_on_shutdown(event, context);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
