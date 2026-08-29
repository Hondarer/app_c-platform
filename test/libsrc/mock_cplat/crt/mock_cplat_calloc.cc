#include <testfw.h>
#include <mock_cplat.h>

void *delegate_real_cplat_calloc(size_t count, size_t size)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_calloc)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_calloc"));

    return real_fn(count, size);
}

MOCK_WEAK_IMPL(void *, cplat_calloc, size_t count, size_t size)
{
    void *mock_ret = nullptr;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_calloc(count, size);
    }
    else
    {
        mock_ret = delegate_real_cplat_calloc(count, size);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %zu %zu -> 0x%p\n", __func__, count, size, mock_ret);
    }

    return mock_ret;
}
