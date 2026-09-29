#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_string_catalog_filter_slot_create(const cplat_string_catalog *catalog,
                                                          const cplat_string_catalog_filter_key_name *key_names,
                                                          size_t key_name_count, size_t line_capacity,
                                                          size_t line_width,
                                                          cplat_string_catalog_filter_slot **slot_out)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_string_catalog_filter_slot_create)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_string_catalog_filter_slot_create"));

    return real_fn(catalog, key_names, key_name_count, line_capacity, line_width, slot_out);
}

MOCK_WEAK_IMPL(int, cplat_string_catalog_filter_slot_create, const cplat_string_catalog *catalog,
               const cplat_string_catalog_filter_key_name *key_names, size_t key_name_count, size_t line_capacity,
               size_t line_width, cplat_string_catalog_filter_slot **slot_out)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_string_catalog_filter_slot_create(catalog, key_names, key_name_count,
                                                                        line_capacity, line_width, slot_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_string_catalog_filter_slot_create(catalog, key_names, key_name_count,
                                                                         line_capacity, line_width, slot_out);
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
