#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

void delegate_real_com_util_etw_provider_dispose(com_util_etw_provider_t *handle)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_etw_provider_dispose)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_etw_provider_dispose"));

    real_fn(handle);
}

MOCK_WEAK_IMPL(void, com_util_etw_provider_dispose, com_util_etw_provider_t *handle)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_etw_provider_dispose(handle);
    }
    else
    {
        delegate_real_com_util_etw_provider_dispose(handle);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p\n", __func__, (void *)handle);
    }
}

#endif /* PLATFORM_WINDOWS */
