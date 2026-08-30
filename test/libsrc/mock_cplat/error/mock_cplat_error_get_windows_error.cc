#include <testfw.h>
#include <mock_cplat.h>

#if defined(PLATFORM_WINDOWS)

unsigned long delegate_real_cplat_error_get_windows_error(const cplat_error *error)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_error_get_windows_error)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_error_get_windows_error"));

    return real_fn(error);
}

MOCK_WEAK_IMPL(unsigned long, cplat_error_get_windows_error, const cplat_error *error)
{
    unsigned long mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_error_get_windows_error(error);
    }
    else
    {
        mock_ret = delegate_real_cplat_error_get_windows_error(error);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }

    return mock_ret;
}

#endif /* PLATFORM_WINDOWS */
