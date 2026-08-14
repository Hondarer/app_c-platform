#include <testfw.h>
#include <mock_com_util.h>

char *delegate_real_com_util_strdup(const char *src)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_strdup)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_strdup"));

    return real_fn(src);
}

MOCK_WEAK_IMPL(char *, com_util_strdup, const char *src)
{
    char *mock_ret = nullptr;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_strdup(src);
    }
    else
    {
        mock_ret = delegate_real_com_util_strdup(src);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s", __func__, src);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
