#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_file_get_size(com_util_file_t *file, size_t *size_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_file_get_size)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_file_get_size"));

    return real_fn(file, size_out);
}

MOCK_WEAK_IMPL(int, com_util_file_get_size, com_util_file_t *file, size_t *size_out)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_file_get_size(file, size_out);
    }
    else
    {
        rtc = delegate_real_com_util_file_get_size(file, size_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, 0x%p", __func__, (void *)file, (void *)size_out);
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
