#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_file_write(com_util_file_t *file, const void *buf, size_t len)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_file_write)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_file_write"));

    return real_fn(file, buf, len);
}

WEAK_ATR int com_util_file_write(com_util_file_t *file, const void *buf, size_t len)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_file_write(file, buf, len);
    }
    else
    {
        rtc = delegate_real_com_util_file_write(file, buf, len);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, 0x%p, %zu", __func__, (void *)file, buf, len);
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
