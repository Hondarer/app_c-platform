#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_string_catalog_vformat(const cplat_string_catalog *catalog, char *dest, size_t dest_size,
                                               int string_key, va_list args)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_string_catalog_vformat)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_string_catalog_vformat"));

    return real_fn(catalog, dest, dest_size, string_key, args);
}

MOCK_WEAK_IMPL(int, cplat_string_catalog_vformat, const cplat_string_catalog *catalog, char *dest, size_t dest_size,
               int string_key, va_list args)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_string_catalog_vformat(catalog, dest, dest_size, string_key, args);
    }
    else
    {
        mock_ret = delegate_real_cplat_string_catalog_vformat(catalog, dest, dest_size, string_key, args);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, 0x%p, %zu, %d", __func__, (const void *)catalog, (void *)dest, dest_size, string_key);
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
