#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_string_catalog_filter_slot_snapshot(cplat_string_catalog_filter_slot *slot, void *image_out,
                                                            size_t image_size)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_string_catalog_filter_slot_snapshot)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_string_catalog_filter_slot_snapshot"));

    return real_fn(slot, image_out, image_size);
}

MOCK_WEAK_IMPL(int, cplat_string_catalog_filter_slot_snapshot, cplat_string_catalog_filter_slot *slot, void *image_out,
               size_t image_size)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_string_catalog_filter_slot_snapshot(slot, image_out, image_size);
    }
    else
    {
        mock_ret = delegate_real_cplat_string_catalog_filter_slot_snapshot(slot, image_out, image_size);
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
