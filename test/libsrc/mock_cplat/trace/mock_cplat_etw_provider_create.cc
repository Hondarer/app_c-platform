#include <testfw.h>
#include <mock_cplat.h>

#if defined(PLATFORM_WINDOWS)

cplat_etw_provider *delegate_real_cplat_etw_provider_create(cplat_etw_provider_ref_t provider_ref)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_etw_provider_create)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_etw_provider_create"));

    return real_fn(provider_ref);
}

MOCK_WEAK_IMPL(cplat_etw_provider *, cplat_etw_provider_create, cplat_etw_provider_ref_t provider_ref)
{
    cplat_etw_provider *mock_ret = nullptr;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_etw_provider_create(provider_ref);
    }
    else
    {
        mock_ret = delegate_real_cplat_etw_provider_create(provider_ref);
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
