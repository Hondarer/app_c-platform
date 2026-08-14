#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_stat(com_util_file_stat_t *buf, com_util_error *detail_out, const char *path)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_stat)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_stat"));

    return real_fn(buf, detail_out, path);
}

MOCK_WEAK_IMPL(int, com_util_stat, com_util_file_stat_t *buf, com_util_error *detail_out, const char *path)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_stat(buf, detail_out, path);
    }
    else
    {
        mock_ret = delegate_real_com_util_stat(buf, detail_out, path);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %s", __func__, (void *)buf, path);
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
