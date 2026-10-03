#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_string_catalog_filter_get_catalog_id(const cplat_string_catalog *catalog,
                                                             uint64_t *catalog_id_out)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_string_catalog_filter_get_catalog_id)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_string_catalog_filter_get_catalog_id"));

    return real_fn(catalog, catalog_id_out);
}

MOCK_WEAK_IMPL(int, cplat_string_catalog_filter_get_catalog_id, const cplat_string_catalog *catalog,
               uint64_t *catalog_id_out)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_string_catalog_filter_get_catalog_id(catalog, catalog_id_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_string_catalog_filter_get_catalog_id(catalog, catalog_id_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
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
