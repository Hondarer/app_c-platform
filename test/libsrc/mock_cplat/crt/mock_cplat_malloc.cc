#include <testfw.h>
#include <mock_cplat.h>

void *delegate_real_cplat_malloc(size_t size)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_malloc)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_malloc"));

    return real_fn(size);
}

MOCK_WEAK_IMPL(void *, cplat_malloc, size_t size)
{
    void *mock_ret = nullptr;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_malloc(size);
    }
    else
    {
        mock_ret = delegate_real_cplat_malloc(size);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %zu -> 0x%p\n", __func__, size, mock_ret);
    }

    return mock_ret;
}
