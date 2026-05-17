#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_file_open(com_util_file_t *file, const char *path, int flags)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_file_open)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_file_open"));

    return real_fn(file, path, flags);
}

MOCK_WEAK_IMPL(int, com_util_file_open, com_util_file_t *file, const char *path, int flags)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_file_open(file, path, flags);
    }
    else
    {
        rtc = delegate_real_com_util_file_open(file, path, flags);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %s, %d", __func__, (void *)file, path, flags);
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
