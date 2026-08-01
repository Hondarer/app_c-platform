#include <testfw.h>
#include <mock_com_util.h>

FILE *delegate_real_com_util_freopen(const char *path, const char *modes, FILE *stream, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_freopen)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_freopen"));

    return real_fn(path, modes, stream, detail_out);
}

MOCK_WEAK_IMPL(FILE *, com_util_freopen, const char *path, const char *modes, FILE *stream, com_util_error *detail_out)
{
    FILE *fp = nullptr;
    const char *path_text = "(null)";
    const char *modes_text = "(null)";

    if (_mock_com_util != nullptr)
    {
        fp = _mock_com_util->com_util_freopen(path, modes, stream, detail_out);
    }
    else
    {
        fp = delegate_real_com_util_freopen(path, modes, stream, detail_out);
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
            printf(" -> 0x%p\n", (void *)fp);
        }
        else
        {
            printf("\n");
        }
    }

    return fp;
}
