#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_stat(com_util_file_stat_t *buf, const char *path)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_stat)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_stat"));

    return real_fn(buf, path);
}

WEAK_ATR int com_util_stat(com_util_file_stat_t *buf, const char *path)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_stat(buf, path);
    }
    else
    {
        rtc = delegate_real_com_util_stat(buf, path);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %s", __func__, (void *)buf, path);
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
