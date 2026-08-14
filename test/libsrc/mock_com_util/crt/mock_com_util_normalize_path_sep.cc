#include <testfw.h>
#include <mock_com_util.h>

char *delegate_real_com_util_normalize_path_sep(char *path)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_normalize_path_sep)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_normalize_path_sep"));

    return real_fn(path);
}

MOCK_WEAK_IMPL(char *, com_util_normalize_path_sep, char *path)
{
    char *mock_ret = nullptr;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_normalize_path_sep(path);
    }
    else
    {
        mock_ret = delegate_real_com_util_normalize_path_sep(path);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s", __func__, (path != nullptr) ? path : "(null)");
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %s\n", (mock_ret != nullptr) ? mock_ret : "(null)");
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
