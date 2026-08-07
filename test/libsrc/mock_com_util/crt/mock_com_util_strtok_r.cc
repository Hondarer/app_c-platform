#include <testfw.h>
#include <mock_com_util.h>

char *delegate_real_com_util_strtok_r(char *str, const char *delim, char **saveptr)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_strtok_r)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_strtok_r"));

    return real_fn(str, delim, saveptr);
}

MOCK_WEAK_IMPL(char *, com_util_strtok_r, char *str, const char *delim, char **saveptr)
{
    char *rtc = nullptr;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_strtok_r(str, delim, saveptr);
    }
    else
    {
        rtc = delegate_real_com_util_strtok_r(str, delim, saveptr);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %s", __func__, (void *)str, delim);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}
