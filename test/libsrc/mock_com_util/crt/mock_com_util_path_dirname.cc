#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_path_dirname(char *path_out, size_t path_size, com_util_error *detail_out, const char *path)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_path_dirname)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_path_dirname"));

    return real_fn(path_out, path_size, detail_out, path);
}

MOCK_WEAK_IMPL(int, com_util_path_dirname, char *path_out, size_t path_size, com_util_error *detail_out,
               const char *path)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_path_dirname(path_out, path_size, detail_out, path);
    }
    else
    {
        rtc = delegate_real_com_util_path_dirname(path_out, path_size, detail_out, path);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        const char *path_text = "(null)";
        if (path != nullptr)
        {
            path_text = path;
        }
        printf("  > %s %s", __func__, path_text);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}
