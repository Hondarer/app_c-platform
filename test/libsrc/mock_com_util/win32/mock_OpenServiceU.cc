#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

SC_HANDLE delegate_real_OpenServiceU(SC_HANDLE scm, const char *utf8_service_name, DWORD desired_access)
{
    static auto real_fn =
        reinterpret_cast<decltype(&OpenServiceU)>(resolveSharedSymbolOrExit(kLibComUtilName, "OpenServiceU"));

    return real_fn(scm, utf8_service_name, desired_access);
}

MOCK_WEAK_IMPL(SC_HANDLE, OpenServiceU, SC_HANDLE scm, const char *utf8_service_name, DWORD desired_access)
{
    SC_HANDLE rtc;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->OpenServiceU(scm, utf8_service_name, desired_access);
    }
    else
    {
        rtc = delegate_real_OpenServiceU(scm, utf8_service_name, desired_access);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %s, %lu", __func__, (void *)scm,
               (utf8_service_name != nullptr) ? utf8_service_name : "(null)", desired_access);
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
