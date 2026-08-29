#include <testfw.h>
#include <mock_cplat.h>

void *delegate_real_cplat_realloc_zerofill(void *ptr, size_t old_count, size_t count, size_t size)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_realloc_zerofill)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_realloc_zerofill"));

    return real_fn(ptr, old_count, count, size);
}

MOCK_WEAK_IMPL(void *, cplat_realloc_zerofill, void *ptr, size_t old_count, size_t count, size_t size)
{
    void *mock_ret = nullptr;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_realloc_zerofill(ptr, old_count, count, size);
    }
    else
    {
        mock_ret = delegate_real_cplat_realloc_zerofill(ptr, old_count, count, size);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p %zu %zu %zu -> 0x%p\n", __func__, ptr, old_count, count, size, mock_ret);
    }

    return mock_ret;
}
