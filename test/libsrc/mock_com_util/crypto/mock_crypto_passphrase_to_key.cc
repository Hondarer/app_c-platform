#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_passphrase_to_key(uint8_t *key, const uint8_t *passphrase,
                                        size_t passphrase_len)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_passphrase_to_key)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_passphrase_to_key"));

    return real_fn(key, passphrase, passphrase_len);
}

MOCK_WEAK_IMPL(int, com_util_passphrase_to_key, uint8_t *key, const uint8_t *passphrase,
                                        size_t passphrase_len)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_passphrase_to_key(key, passphrase, passphrase_len);
    }
    else
    {
        rtc = delegate_real_com_util_passphrase_to_key(key, passphrase, passphrase_len);
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
