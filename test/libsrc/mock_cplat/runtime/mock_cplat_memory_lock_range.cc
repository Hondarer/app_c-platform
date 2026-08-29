#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_memory_lock_range(const void *address, size_t size)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_memory_lock_range)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_memory_lock_range"));

    return real_fn(address, size);
}

MOCK_WEAK_IMPL(int, cplat_memory_lock_range, const void *address, size_t size)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_memory_lock_range(address, size);
    }
    else
    {
        mock_ret = delegate_real_cplat_memory_lock_range(address, size);
    }

    return mock_ret;
}
