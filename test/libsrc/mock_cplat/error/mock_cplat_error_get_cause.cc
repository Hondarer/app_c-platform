#include <testfw.h>
#include <mock_cplat.h>

cplat_error_cause delegate_real_cplat_error_get_cause(const cplat_error *error)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_error_get_cause)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_error_get_cause"));

    return real_fn(error);
}

MOCK_WEAK_IMPL(cplat_error_cause, cplat_error_get_cause, const cplat_error *error)
{
    cplat_error_cause mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_error_get_cause(error);
    }
    else
    {
        mock_ret = delegate_real_cplat_error_get_cause(error);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }

    return mock_ret;
}
