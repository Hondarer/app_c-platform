#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_compress(uint8_t *dst, size_t *dst_len,
                               const uint8_t *src, size_t src_len)
{
    /* TODO: com_util.h を動的に読み込み、com_util_compress@libcom_util を呼び出し、戻り値にする。app/com_util/prod/libsrc/com_util/runtime と同様の機構を用いれば実現できると思うが、テストフレームワークは製品と別なので、共通機能が必要であれば framework/testfw/libsrc/test_com を拡張してそれを用いたい。 */
    return 0;
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
