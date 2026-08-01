#include <testfw.h>
#include <mock_com_util.h>

#include <inttypes.h>

int64_t delegate_real_com_util_lseek(int fd, int64_t offset, int whence, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_lseek)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_lseek"));

    return real_fn(fd, offset, whence, detail_out);
}

MOCK_WEAK_IMPL(int64_t, com_util_lseek, int fd, int64_t offset, int whence, com_util_error *detail_out)
{
    int64_t rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_lseek(fd, offset, whence, detail_out);
    }
    else
    {
        rtc = delegate_real_com_util_lseek(fd, offset, whence, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d, %" PRId64 ", %d", __func__, fd, offset, whence);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %" PRId64 "\n", rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}
