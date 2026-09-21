#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_ui_language_get_tag(char *tag_out, size_t tag_size)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_ui_language_get_tag)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_ui_language_get_tag"));

    return real_fn(tag_out, tag_size);
}

MOCK_WEAK_IMPL(int, cplat_ui_language_get_tag, char *tag_out, size_t tag_size)
{
    int mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_ui_language_get_tag(tag_out, tag_size);
    }
    else
    {
        mock_ret = delegate_real_cplat_ui_language_get_tag(tag_out, tag_size);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu", __func__, (void *)tag_out, tag_size);
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
