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
    char *rtc = nullptr;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_normalize_path_sep(path);
    }
    else
    {
        rtc = delegate_real_com_util_normalize_path_sep(path);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s", __func__, (path != nullptr) ? path : "(null)");
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %s\n", (rtc != nullptr) ? rtc : "(null)");
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}
