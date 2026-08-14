#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

com_util_etw_provider *delegate_real_com_util_etw_provider_create(com_util_etw_provider_ref_t provider_ref)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_etw_provider_create)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_etw_provider_create"));

    return real_fn(provider_ref);
}

MOCK_WEAK_IMPL(com_util_etw_provider *, com_util_etw_provider_create, com_util_etw_provider_ref_t provider_ref)
{
    com_util_etw_provider *mock_ret = nullptr;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_etw_provider_create(provider_ref);
    }
    else
    {
        mock_ret = delegate_real_com_util_etw_provider_create(provider_ref);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (void *)provider_ref);
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
