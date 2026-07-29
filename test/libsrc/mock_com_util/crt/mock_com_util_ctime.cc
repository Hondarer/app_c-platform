#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_ctime(char *buf, size_t buf_size, const time_t *timep)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_ctime)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_ctime"));

    return real_fn(buf, buf_size, timep);
}

MOCK_WEAK_IMPL(int, com_util_ctime, char *buf, size_t buf_size, const time_t *timep)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_ctime(buf, buf_size, timep);
    }
    else
    {
        rtc = delegate_real_com_util_ctime(buf, buf_size, timep);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu, 0x%p", __func__, (void *)buf, buf_size, (const void *)timep);
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
