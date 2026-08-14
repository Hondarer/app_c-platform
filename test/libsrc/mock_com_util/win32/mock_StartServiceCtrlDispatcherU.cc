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
    BOOL mock_ret;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->StartServiceCtrlDispatcherU(service_table);
    }
    else
    {
        mock_ret = delegate_real_StartServiceCtrlDispatcherU(service_table);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (const void *)service_table);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}

#endif /* PLATFORM_WINDOWS */
