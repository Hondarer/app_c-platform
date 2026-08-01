#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_file_write(com_util_file *file, const void *buf, size_t len, com_util_error *detail_out)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_file_write)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_file_write"));

    return real_fn(file, buf, len, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_file_write, com_util_file *file, const void *buf, size_t len, com_util_error *detail_out)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_file_write(file, buf, len, detail_out);
    }
    else
    {
        rtc = delegate_real_com_util_file_write(file, buf, len, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, 0x%p, %zu", __func__, (void *)file, buf, len);
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
