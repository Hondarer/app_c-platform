#include <testfw.h>
#include <mock_cplat.h>

size_t delegate_real_cplat_regex_get_group_count(const cplat_regex *regex)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_regex_get_group_count)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_regex_get_group_count"));

    return real_fn(regex);
}

MOCK_WEAK_IMPL(size_t, cplat_regex_get_group_count, const cplat_regex *regex)
{
    size_t mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_regex_get_group_count(regex);
    }
    else
    {
        mock_ret = delegate_real_cplat_regex_get_group_count(regex);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }

    return mock_ret;
}
