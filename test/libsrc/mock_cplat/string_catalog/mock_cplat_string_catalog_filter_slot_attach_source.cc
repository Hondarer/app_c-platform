#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_string_catalog_filter_slot_attach_source(cplat_string_catalog_filter_slot *slot,
                                                                 const void *source, size_t source_size,
                                                                 const cplat_string_catalog_filter_source_lock *lock)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_string_catalog_filter_slot_attach_source)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_string_catalog_filter_slot_attach_source"));

    return real_fn(slot, source, source_size, lock);
}

MOCK_WEAK_IMPL(int, cplat_string_catalog_filter_slot_attach_source, cplat_string_catalog_filter_slot *slot,
               const void *source, size_t source_size, const cplat_string_catalog_filter_source_lock *lock)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_string_catalog_filter_slot_attach_source(slot, source, source_size, lock);
    }
    else
    {
        mock_ret = delegate_real_cplat_string_catalog_filter_slot_attach_source(slot, source, source_size, lock);
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
