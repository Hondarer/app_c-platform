#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_string_catalog_language_from_tag(const char *tag, cplat_string_catalog_language *language_out)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_string_catalog_language_from_tag)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_string_catalog_language_from_tag"));

    return real_fn(tag, language_out);
}

MOCK_WEAK_IMPL(int, cplat_string_catalog_language_from_tag, const char *tag,
               cplat_string_catalog_language *language_out)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_string_catalog_language_from_tag(tag, language_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_string_catalog_language_from_tag(tag, language_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, 0x%p", __func__, tag != nullptr ? tag : "(null)", (void *)language_out);
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
