#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_string_catalog_filter_slot_vformat(cplat_string_catalog_filter_slot *slot, char *dest,
                                                           size_t dest_size, int *matched_out, int string_key,
                                                           va_list args)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_string_catalog_filter_slot_vformat)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_string_catalog_filter_slot_vformat"));

    return real_fn(slot, dest, dest_size, matched_out, string_key, args);
}

MOCK_WEAK_IMPL(int, cplat_string_catalog_filter_slot_vformat, cplat_string_catalog_filter_slot *slot, char *dest,
               size_t dest_size, int *matched_out, int string_key, va_list args)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret =
            _mock_cplat->cplat_string_catalog_filter_slot_vformat(slot, dest, dest_size, matched_out, string_key, args);
    }
    else
    {
        mock_ret = delegate_real_cplat_string_catalog_filter_slot_vformat(slot, dest, dest_size, matched_out,
                                                                          string_key, args);
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
