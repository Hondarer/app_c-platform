#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_strcat(char *dest, size_t dest_size, const char *src)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_strcat)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_strcat"));

    return real_fn(dest, dest_size, src);
}

MOCK_WEAK_IMPL(int, com_util_strcat, char *dest, size_t dest_size, const char *src)
{
    int mock_ret = -1;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_strcat(dest, dest_size, src);
    }
    else
    {
        mock_ret = delegate_real_com_util_strcat(dest, dest_size, src);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu, %s", __func__, (void *)dest, dest_size, src);
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
