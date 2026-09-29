#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_string_catalog_filter_insert_line(void *image, size_t image_size, size_t line_index,
                                                          const char *text,
                                                          cplat_string_catalog_filter_diagnostic *diagnostic_out)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_string_catalog_filter_insert_line)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_string_catalog_filter_insert_line"));

    return real_fn(image, image_size, line_index, text, diagnostic_out);
}

MOCK_WEAK_IMPL(int, cplat_string_catalog_filter_insert_line, void *image, size_t image_size, size_t line_index,
               const char *text, cplat_string_catalog_filter_diagnostic *diagnostic_out)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret =
            _mock_cplat->cplat_string_catalog_filter_insert_line(image, image_size, line_index, text, diagnostic_out);
    }
    else
    {
        mock_ret =
            delegate_real_cplat_string_catalog_filter_insert_line(image, image_size, line_index, text, diagnostic_out);
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
