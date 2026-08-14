#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_remove(const char *path, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_remove)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_remove"));

    return real_fn(path, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_remove, const char *path, com_util_error *detail_out)
{
    int mock_ret = -1;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_remove(path, detail_out);
    }
    else
    {
        mock_ret = delegate_real_com_util_remove(path, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s", __func__, path);
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
