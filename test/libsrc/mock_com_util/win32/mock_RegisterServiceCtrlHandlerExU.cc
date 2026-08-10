#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

SERVICE_STATUS_HANDLE delegate_real_RegisterServiceCtrlHandlerExU(const char *utf8_service_name,
                                                                  LPHANDLER_FUNCTION_EX handler_proc, LPVOID context)
{
    static auto real_fn = reinterpret_cast<decltype(&RegisterServiceCtrlHandlerExU)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "RegisterServiceCtrlHandlerExU"));

    return real_fn(utf8_service_name, handler_proc, context);
}

MOCK_WEAK_IMPL(SERVICE_STATUS_HANDLE, RegisterServiceCtrlHandlerExU, const char *utf8_service_name,
               LPHANDLER_FUNCTION_EX handler_proc, LPVOID context)
{
    SERVICE_STATUS_HANDLE rtc;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->RegisterServiceCtrlHandlerExU(utf8_service_name, handler_proc, context);
    }
    else
    {
        rtc = delegate_real_RegisterServiceCtrlHandlerExU(utf8_service_name, handler_proc, context);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, 0x%p, 0x%p", __func__, (utf8_service_name != nullptr) ? utf8_service_name : "(null)",
               (void *)handler_proc, (void *)context);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}

#endif /* PLATFORM_WINDOWS */
