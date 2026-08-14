#include <testfw.h>
#include <mock_com_util.h>

void *delegate_real_com_util_malloc(size_t size)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_malloc)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_malloc"));

    return real_fn(size);
}

MOCK_WEAK_IMPL(void *, com_util_malloc, size_t size)
{
    void *ptr = nullptr;

    if (_mock_com_util != nullptr)
    {
        ptr = _mock_com_util->com_util_malloc(size);
    }
    else
    {
        ptr = delegate_real_com_util_malloc(size);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %zu -> 0x%p\n", __func__, size, ptr);
    }

    return ptr;
}
