#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_decompress(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len)
{
    static auto real_decompress = reinterpret_cast<decltype(&com_util_decompress)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_decompress"));

    return real_decompress(dst, dst_len, src, src_len);
}

MOCK_WEAK_IMPL(int, com_util_decompress, uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_decompress(dst, dst_len, src, src_len);
    }
    else
    {
        mock_ret = delegate_real_com_util_decompress(dst, dst_len, src, src_len);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
