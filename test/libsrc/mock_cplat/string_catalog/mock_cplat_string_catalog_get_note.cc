#include <testfw.h>
#include <mock_cplat.h>

const char *delegate_real_cplat_string_catalog_get_note(const cplat_string_catalog *catalog, int string_key)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_string_catalog_get_note)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_string_catalog_get_note"));

    return real_fn(catalog, string_key);
}

MOCK_WEAK_IMPL(const char *, cplat_string_catalog_get_note, const cplat_string_catalog *catalog, int string_key)
{
    const char *mock_ret = nullptr;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_string_catalog_get_note(catalog, string_key);
    }
    else
    {
        mock_ret = delegate_real_cplat_string_catalog_get_note(catalog, string_key);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %d", __func__, (const void *)catalog, string_key);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %s\n", mock_ret != nullptr ? mock_ret : "(null)");
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
