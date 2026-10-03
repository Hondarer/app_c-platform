#include <testfw.h>
#include <mock_cplat.h>

const cplat_string_catalog *
delegate_real_cplat_string_catalog_filter_slot_get_catalog(const cplat_string_catalog_filter_slot *slot)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_string_catalog_filter_slot_get_catalog)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_string_catalog_filter_slot_get_catalog"));

    return real_fn(slot);
}

MOCK_WEAK_IMPL(const cplat_string_catalog *, cplat_string_catalog_filter_slot_get_catalog,
               const cplat_string_catalog_filter_slot *slot)
{
    const cplat_string_catalog *mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_string_catalog_filter_slot_get_catalog(slot);
    }
    else
    {
        mock_ret = delegate_real_cplat_string_catalog_filter_slot_get_catalog(slot);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %p\n", (const void *)mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
