#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_rename(const char *oldpath, const char *newpath, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_rename)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_rename"));

    return real_fn(oldpath, newpath, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_rename, const char *oldpath, const char *newpath, com_util_error *detail_out)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_rename(oldpath, newpath, detail_out);
    }
    else
    {
        rtc = delegate_real_com_util_rename(oldpath, newpath, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, %s", __func__, oldpath, newpath);
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
