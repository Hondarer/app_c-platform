#include <testfw.h>
#include <mock_cplat.h>

cplat_string_catalog_language delegate_real_cplat_string_catalog_get_language(void)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_string_catalog_get_language)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_string_catalog_get_language"));

    return real_fn();
}

MOCK_WEAK_IMPL(cplat_string_catalog_language, cplat_string_catalog_get_language, void)
{
    cplat_string_catalog_language mock_ret = CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_string_catalog_get_language();
    }
    else
    {
        mock_ret = delegate_real_cplat_string_catalog_get_language();
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", (int)mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
