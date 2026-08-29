#include <testfw.h>
#include <mock_cplat.h>

#include <inttypes.h>

int64_t delegate_real_cplat_lseek(int fd, int64_t offset, int whence, cplat_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_lseek)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_lseek"));

    return real_fn(fd, offset, whence, detail_out);
}

MOCK_WEAK_IMPL(int64_t, cplat_lseek, int fd, int64_t offset, int whence, cplat_error *detail_out)
{
    int64_t mock_ret = -1;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_lseek(fd, offset, whence, detail_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_lseek(fd, offset, whence, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d, %" PRId64 ", %d", __func__, fd, offset, whence);
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
