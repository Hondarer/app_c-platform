#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_string_catalog_filter_slot_set_category_names(
    cplat_string_catalog_filter_slot *slot, const cplat_string_catalog_filter_category_names *category_names)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_string_catalog_filter_slot_set_category_names)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_string_catalog_filter_slot_set_category_names"));

    return real_fn(slot, category_names);
}

MOCK_WEAK_IMPL(int, cplat_string_catalog_filter_slot_set_category_names, cplat_string_catalog_filter_slot *slot,
               const cplat_string_catalog_filter_category_names *category_names)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_string_catalog_filter_slot_set_category_names(slot, category_names);
    }
    else
    {
        mock_ret = delegate_real_cplat_string_catalog_filter_slot_set_category_names(slot, category_names);
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
