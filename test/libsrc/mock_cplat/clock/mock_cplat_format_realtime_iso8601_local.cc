#include <testfw.h>
#include <mock_cplat.h>
#include <inttypes.h>

int delegate_real_cplat_format_realtime_iso8601_local(char *buf, size_t buf_size, const cplat_timespec *timestamp)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_format_realtime_iso8601_local)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_format_realtime_iso8601_local"));

    return real_fn(buf, buf_size, timestamp);
}

MOCK_WEAK_IMPL(int, cplat_format_realtime_iso8601_local, char *buf, size_t buf_size,
               const cplat_timespec *timestamp)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_format_realtime_iso8601_local(buf, buf_size, timestamp);
    }
    else
    {
        mock_ret = delegate_real_cplat_format_realtime_iso8601_local(buf, buf_size, timestamp);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu, 0x%p", __func__, (void *)buf, buf_size, (const void *)timestamp);
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
