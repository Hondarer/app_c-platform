#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_compress(uint8_t *dst, size_t *dst_len,
                                    const uint8_t *src, size_t src_len)
{
    static auto real_compress =
        reinterpret_cast<decltype(&com_util_compress)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_compress"));

    return real_compress(dst, dst_len, src, src_len);
}

WEAK_ATR int com_util_compress(uint8_t *dst, size_t *dst_len,
                               const uint8_t *src, size_t src_len)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_compress(dst, dst_len, src, src_len);
    }
    else
    {
        rtc = delegate_real_com_util_compress(dst, dst_len, src, src_len);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
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
