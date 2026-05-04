#include <testfw.h>
#include <mock_com_util.h>

size_t delegate_real_com_util_fwrite(const void *ptr, size_t size, size_t count, FILE *stream)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_fwrite)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_fwrite"));

    return real_fn(ptr, size, count, stream);
}

MOCK_WEAK_IMPL(size_t, com_util_fwrite, const void *ptr, size_t size, size_t count, FILE *stream)
{
    size_t rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_fwrite(ptr, size, count, stream);
    }
    else
    {
        rtc = delegate_real_com_util_fwrite(ptr, size, count, stream);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu, %zu, 0x%p", __func__, (const void *)ptr, size, count, (void *)stream);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %zu\n", rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}
