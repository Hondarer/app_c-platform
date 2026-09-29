#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_string_catalog_filter_slot_dispose(cplat_string_catalog_filter_slot **slot)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_string_catalog_filter_slot_dispose)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_string_catalog_filter_slot_dispose"));

    real_fn(slot);
}

MOCK_WEAK_IMPL(void, cplat_string_catalog_filter_slot_dispose, cplat_string_catalog_filter_slot **slot)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_string_catalog_filter_slot_dispose(slot);
    }
    else
    {
        delegate_real_cplat_string_catalog_filter_slot_dispose(slot);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
