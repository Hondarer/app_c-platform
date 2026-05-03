#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_strcpy(char *dest, size_t dest_size, const char *src)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_strcpy)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_strcpy"));

    return real_fn(dest, dest_size, src);
}

WEAK_ATR int com_util_strcpy(char *dest, size_t dest_size, const char *src)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_strcpy(dest, dest_size, src);
    }
    else
    {
        rtc = delegate_real_com_util_strcpy(dest, dest_size, src);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu, %s", __func__, (void *)dest, dest_size, src);
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
