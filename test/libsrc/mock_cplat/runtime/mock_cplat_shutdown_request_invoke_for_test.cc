#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_shutdown_request_invoke_for_test(const cplat_shutdown_event *event, int *invoked_out)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_shutdown_request_invoke_for_test)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_shutdown_request_invoke_for_test"));

    return real_fn(event, invoked_out);
}

MOCK_WEAK_IMPL(int, cplat_shutdown_request_invoke_for_test, const cplat_shutdown_event *event, int *invoked_out)
{
    int mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_shutdown_request_invoke_for_test(event, invoked_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_shutdown_request_invoke_for_test(event, invoked_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }

    return mock_ret;
}
