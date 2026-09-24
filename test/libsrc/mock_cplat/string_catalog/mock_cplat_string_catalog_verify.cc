#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_string_catalog_verify(const cplat_string_catalog *catalog, int *string_key_out,
                                              cplat_string_catalog_language *language_out)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_string_catalog_verify)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_string_catalog_verify"));

    return real_fn(catalog, string_key_out, language_out);
}

MOCK_WEAK_IMPL(int, cplat_string_catalog_verify, const cplat_string_catalog *catalog, int *string_key_out,
               cplat_string_catalog_language *language_out)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_string_catalog_verify(catalog, string_key_out, language_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_string_catalog_verify(catalog, string_key_out, language_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, 0x%p, 0x%p", __func__, (const void *)catalog, (void *)string_key_out,
               (void *)language_out);
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
