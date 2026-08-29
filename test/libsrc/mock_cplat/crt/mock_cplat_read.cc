#include <testfw.h>
#include <mock_cplat.h>

#include <inttypes.h>

int64_t delegate_real_cplat_read(int fd, void *buf, size_t count, cplat_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_read)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_read"));

    return real_fn(fd, buf, count, detail_out);
}

MOCK_WEAK_IMPL(int64_t, cplat_read, int fd, void *buf, size_t count, cplat_error *detail_out)
{
    int64_t mock_ret = -1;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_read(fd, buf, count, detail_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_read(fd, buf, count, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d, 0x%p, %zu", __func__, fd, buf, count);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %" PRId64 "\n", mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
