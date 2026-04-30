#include <testfw.h>
#include <mock_com_util.h>

WEAK_ATR int com_util_getenv(const char *name, char *buf, size_t buf_size)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_getenv(name, buf, buf_size);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, 0x%p, %zu", __func__, name, (void *)buf, buf_size);
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
