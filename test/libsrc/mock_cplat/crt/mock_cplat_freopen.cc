#include <testfw.h>
#include <mock_cplat.h>

FILE *delegate_real_cplat_freopen(const char *path, const char *modes, FILE *stream, cplat_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_freopen)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_freopen"));

    return real_fn(path, modes, stream, detail_out);
}

MOCK_WEAK_IMPL(FILE *, cplat_freopen, const char *path, const char *modes, FILE *stream, cplat_error *detail_out)
{
    FILE *mock_ret = nullptr;
    const char *path_text = "(null)";
    const char *modes_text = "(null)";

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_freopen(path, modes, stream, detail_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_freopen(path, modes, stream, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        if (path != nullptr)
        {
            path_text = path;
        }
        if (modes != nullptr)
        {
            modes_text = modes;
        }

        printf("  > %s %s, %s, 0x%p, 0x%p", __func__, path_text, modes_text, (void *)stream, (void *)detail_out);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
