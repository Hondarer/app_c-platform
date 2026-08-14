#include <testfw.h>
#include <mock_com_util.h>

void *delegate_real_com_util_calloc(size_t count, size_t size)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_calloc)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_calloc"));

    return real_fn(count, size);
}

MOCK_WEAK_IMPL(void *, com_util_calloc, size_t count, size_t size)
{
    void *ptr = nullptr;

    if (_mock_com_util != nullptr)
    {
        ptr = _mock_com_util->com_util_calloc(count, size);
    }
    else
    {
        ptr = delegate_real_com_util_calloc(count, size);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %zu %zu -> 0x%p\n", __func__, count, size, ptr);
    }

    return ptr;
}
