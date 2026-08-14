#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_paths_equal(const char *lhs, const char *rhs, int *equal_out, com_util_error *detail_out)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_paths_equal)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_paths_equal"));

    return real_fn(lhs, rhs, equal_out, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_paths_equal, const char *lhs, const char *rhs, int *equal_out, com_util_error *detail_out)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_paths_equal(lhs, rhs, equal_out, detail_out);
    }
    else
    {
        mock_ret = delegate_real_com_util_paths_equal(lhs, rhs, equal_out, detail_out);
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
            printf(" -> %d\n", mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
