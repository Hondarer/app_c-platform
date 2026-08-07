#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_fgets(char *dest, size_t dest_size, FILE *stream, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_fgets)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_fgets"));

    return real_fn(dest, dest_size, stream, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_fgets, char *dest, size_t dest_size, FILE *stream, com_util_error *detail_out)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_fgets(dest, dest_size, stream, detail_out);
    }
    else
    {
        rtc = delegate_real_com_util_fgets(dest, dest_size, stream, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu, 0x%p", __func__, (void *)dest, dest_size, (void *)stream);
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
