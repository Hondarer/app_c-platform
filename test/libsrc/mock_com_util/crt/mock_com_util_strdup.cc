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
    char *dup = nullptr;

    if (_mock_com_util != nullptr)
    {
        dup = _mock_com_util->com_util_strdup(src);
    }
    else
    {
        dup = delegate_real_com_util_strdup(src);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s", __func__, src);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)dup);
        }
        else
        {
            printf("\n");
        }
    }

    return dup;
}
