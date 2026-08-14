#include <testfw.h>
#include <mock_com_util.h>

void *delegate_real_com_util_realloc(void *ptr, size_t count, size_t size)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_realloc)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_realloc"));

    return real_fn(ptr, count, size);
}

MOCK_WEAK_IMPL(void *, com_util_realloc, void *ptr, size_t count, size_t size)
{
    void *mock_ret = nullptr;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_realloc(ptr, count, size);
    }
    else
    {
        mock_ret = delegate_real_com_util_realloc(ptr, count, size);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p %zu %zu -> 0x%p\n", __func__, ptr, count, size, mock_ret);
    }

    return mock_ret;
}
