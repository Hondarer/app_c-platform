#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_compress(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len)
{
    static auto real_compress =
        reinterpret_cast<decltype(&cplat_compress)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_compress"));

    return real_compress(dst, dst_len, src, src_len);
}

MOCK_WEAK_IMPL(int, cplat_compress, uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_compress(dst, dst_len, src, src_len);
    }
    else
    {
        mock_ret = delegate_real_cplat_compress(dst, dst_len, src, src_len);
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
