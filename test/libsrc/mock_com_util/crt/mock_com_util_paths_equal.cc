#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_paths_equal(const char *lhs, const char *rhs, int *equal_out, int *errno_out)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_paths_equal)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_paths_equal"));

    return real_fn(lhs, rhs, equal_out, errno_out);
}

MOCK_WEAK_IMPL(int, com_util_paths_equal, const char *lhs, const char *rhs, int *equal_out, int *errno_out)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_paths_equal(lhs, rhs, equal_out, errno_out);
    }
    else
    {
        rtc = delegate_real_com_util_paths_equal(lhs, rhs, equal_out, errno_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        const char *lhs_text = "(null)";
        const char *rhs_text = "(null)";
        if (lhs != nullptr)
        {
            lhs_text = lhs;
        }
        if (rhs != nullptr)
        {
            rhs_text = rhs;
        }
        printf("  > %s %s %s", __func__, lhs_text, rhs_text);
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
