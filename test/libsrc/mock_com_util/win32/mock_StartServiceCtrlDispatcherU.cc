#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

BOOL delegate_real_StartServiceCtrlDispatcherU(const com_util_service_entry_u *service_table)
{
    static auto real_fn = reinterpret_cast<decltype(&StartServiceCtrlDispatcherU)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "StartServiceCtrlDispatcherU"));

    return real_fn(service_table);
}

MOCK_WEAK_IMPL(BOOL, StartServiceCtrlDispatcherU, const com_util_service_entry_u *service_table)
{
    BOOL rtc;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->StartServiceCtrlDispatcherU(service_table);
    }
    else
    {
        rtc = delegate_real_StartServiceCtrlDispatcherU(service_table);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (const void *)service_table);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}

#endif /* PLATFORM_WINDOWS */
