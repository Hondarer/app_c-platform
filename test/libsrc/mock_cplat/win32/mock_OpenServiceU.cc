#include <testfw.h>
#include <mock_cplat.h>

#if defined(PLATFORM_WINDOWS)

SC_HANDLE delegate_real_OpenServiceU(SC_HANDLE scm, const char *utf8_service_name, DWORD desired_access)
{
    static auto real_fn =
        reinterpret_cast<decltype(&OpenServiceU)>(resolveSharedSymbolOrExit(kLibCplatName, "OpenServiceU"));

    return real_fn(scm, utf8_service_name, desired_access);
}

MOCK_WEAK_IMPL(SC_HANDLE, OpenServiceU, SC_HANDLE scm, const char *utf8_service_name, DWORD desired_access)
{
    SC_HANDLE mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->OpenServiceU(scm, utf8_service_name, desired_access);
    }
    else
    {
        mock_ret = delegate_real_OpenServiceU(scm, utf8_service_name, desired_access);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %s, %lu", __func__, (void *)scm,
               (utf8_service_name != nullptr) ? utf8_service_name : "(null)", desired_access);
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
