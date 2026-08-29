#include <testfw.h>
#include <mock_cplat.h>

#if defined(PLATFORM_WINDOWS)

SERVICE_STATUS_HANDLE delegate_real_RegisterServiceCtrlHandlerExU(const char *utf8_service_name,
                                                                  LPHANDLER_FUNCTION_EX handler_proc, LPVOID context)
{
    static auto real_fn = reinterpret_cast<decltype(&RegisterServiceCtrlHandlerExU)>(
        resolveSharedSymbolOrExit(kLibCplatName, "RegisterServiceCtrlHandlerExU"));

    return real_fn(utf8_service_name, handler_proc, context);
}

MOCK_WEAK_IMPL(SERVICE_STATUS_HANDLE, RegisterServiceCtrlHandlerExU, const char *utf8_service_name,
               LPHANDLER_FUNCTION_EX handler_proc, LPVOID context)
{
    SERVICE_STATUS_HANDLE mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->RegisterServiceCtrlHandlerExU(utf8_service_name, handler_proc, context);
    }
    else
    {
        mock_ret = delegate_real_RegisterServiceCtrlHandlerExU(utf8_service_name, handler_proc, context);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, 0x%p, 0x%p", __func__, (utf8_service_name != nullptr) ? utf8_service_name : "(null)",
               (void *)handler_proc, (void *)context);
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

#endif /* PLATFORM_WINDOWS */
