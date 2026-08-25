#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_file_stat_is_regular(const com_util_file_stat_t *file_stat)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_file_stat_is_regular)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_file_stat_is_regular"));

    return real_fn(file_stat);
}

MOCK_WEAK_IMPL(int, com_util_file_stat_is_regular, const com_util_file_stat_t *file_stat)
{
    int mock_ret = 0;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_file_stat_is_regular(file_stat);
    }
    else
    {
        mock_ret = delegate_real_com_util_file_stat_is_regular(file_stat);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (const void *)file_stat);
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
