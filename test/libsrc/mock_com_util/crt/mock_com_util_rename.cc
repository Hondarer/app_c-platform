#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_rename(const char *oldpath, const char *newpath, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_rename)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_rename"));

    return real_fn(oldpath, newpath, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_rename, const char *oldpath, const char *newpath, com_util_error *detail_out)
{
    int mock_ret = -1;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_rename(oldpath, newpath, detail_out);
    }
    else
    {
        mock_ret = delegate_real_com_util_rename(oldpath, newpath, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, %s", __func__, oldpath, newpath);
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
