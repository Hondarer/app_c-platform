#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_timespec_cmp(const com_util_timespec *a, const com_util_timespec *b)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_timespec_cmp)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_timespec_cmp"));

    return real_fn(a, b);
}

MOCK_WEAK_IMPL(int, com_util_timespec_cmp, const com_util_timespec *a, const com_util_timespec *b)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_timespec_cmp(a, b);
    }
    else
    {
        mock_ret = delegate_real_com_util_timespec_cmp(a, b);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
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
