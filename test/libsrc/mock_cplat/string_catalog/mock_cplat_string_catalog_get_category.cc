#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_string_catalog_get_category(const cplat_string_catalog *catalog, int string_key)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_string_catalog_get_category)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_string_catalog_get_category"));

    return real_fn(catalog, string_key);
}

MOCK_WEAK_IMPL(int, cplat_string_catalog_get_category, const cplat_string_catalog *catalog, int string_key)
{
    int mock_ret = 0;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_string_catalog_get_category(catalog, string_key);
    }
    else
    {
        mock_ret = delegate_real_cplat_string_catalog_get_category(catalog, string_key);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %d", __func__, (const void *)catalog, string_key);
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
