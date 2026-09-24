#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_string_catalog_set_language(cplat_string_catalog_language language)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_string_catalog_set_language)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_string_catalog_set_language"));

    return real_fn(language);
}

MOCK_WEAK_IMPL(int, cplat_string_catalog_set_language, cplat_string_catalog_language language)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_string_catalog_set_language(language);
    }
    else
    {
        mock_ret = delegate_real_cplat_string_catalog_set_language(language);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d", __func__, (int)language);
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
