#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_decrypt(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len,
                                   const uint8_t *key, const uint8_t *nonce, const uint8_t *aad, size_t aad_len)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_decrypt)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_decrypt"));

    return real_fn(dst, dst_len, src, src_len, key, nonce, aad, aad_len);
}

MOCK_WEAK_IMPL(int, com_util_decrypt, uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len,
               const uint8_t *key, const uint8_t *nonce, const uint8_t *aad, size_t aad_len)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_decrypt(dst, dst_len, src, src_len, key, nonce, aad, aad_len);
    }
    else
    {
        rtc = delegate_real_com_util_decrypt(dst, dst_len, src, src_len, key, nonce, aad, aad_len);
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
