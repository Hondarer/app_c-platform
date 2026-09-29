#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_string_catalog_filter_remove_line(void *image, size_t image_size, size_t line_index)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_string_catalog_filter_remove_line)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_string_catalog_filter_remove_line"));

    return real_fn(image, image_size, line_index);
}

MOCK_WEAK_IMPL(int, cplat_string_catalog_filter_remove_line, void *image, size_t image_size, size_t line_index)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_string_catalog_filter_remove_line(image, image_size, line_index);
    }
    else
    {
        mock_ret = delegate_real_cplat_string_catalog_filter_remove_line(image, image_size, line_index);
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
