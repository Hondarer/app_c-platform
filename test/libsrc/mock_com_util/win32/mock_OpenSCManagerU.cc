#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

SC_HANDLE delegate_real_OpenSCManagerU(const char *utf8_machine_name, const char *utf8_database_name,
                                       DWORD desired_access)
{
    static auto real_fn =
        reinterpret_cast<decltype(&OpenSCManagerU)>(resolveSharedSymbolOrExit(kLibComUtilName, "OpenSCManagerU"));

    return real_fn(utf8_machine_name, utf8_database_name, desired_access);
}

MOCK_WEAK_IMPL(SC_HANDLE, OpenSCManagerU, const char *utf8_machine_name, const char *utf8_database_name,
               DWORD desired_access)
{
    SC_HANDLE mock_ret;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->OpenSCManagerU(utf8_machine_name, utf8_database_name, desired_access);
    }
    else
    {
        mock_ret = delegate_real_OpenSCManagerU(utf8_machine_name, utf8_database_name, desired_access);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, %s, %lu", __func__, (utf8_machine_name != nullptr) ? utf8_machine_name : "(null)",
               (utf8_database_name != nullptr) ? utf8_database_name : "(null)", desired_access);
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
