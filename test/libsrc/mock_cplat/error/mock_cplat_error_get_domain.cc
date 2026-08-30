#include <testfw.h>
#include <mock_cplat.h>

cplat_error_domain delegate_real_cplat_error_get_domain(const cplat_error *error)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_error_get_domain)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_error_get_domain"));

    return real_fn(error);
}

MOCK_WEAK_IMPL(cplat_error_domain, cplat_error_get_domain, const cplat_error *error)
{
    cplat_error_domain mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_error_get_domain(error);
    }
    else
    {
        mock_ret = delegate_real_cplat_error_get_domain(error);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }

    return mock_ret;
}
