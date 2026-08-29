#include <testfw.h>
#include <mock_cplat.h>

#if defined(PLATFORM_WINDOWS)

void delegate_real_cplat_etw_provider_dispose(cplat_etw_provider *handle)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_etw_provider_dispose)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_etw_provider_dispose"));

    real_fn(handle);
}

MOCK_WEAK_IMPL(void, cplat_etw_provider_dispose, cplat_etw_provider *handle)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_etw_provider_dispose(handle);
    }
    else
    {
        delegate_real_cplat_etw_provider_dispose(handle);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p\n", __func__, (void *)handle);
    }
}

#endif /* PLATFORM_WINDOWS */
