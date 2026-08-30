#include <testfw.h>
#include <mock_cplat.h>

size_t delegate_real_cplat_mmap_get_size(const cplat_mmap *map)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_mmap_get_size)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_mmap_get_size"));

    return real_fn(map);
}

MOCK_WEAK_IMPL(size_t, cplat_mmap_get_size, const cplat_mmap *map)
{
    size_t mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_mmap_get_size(map);
    }
    else
    {
        mock_ret = delegate_real_cplat_mmap_get_size(map);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }

    return mock_ret;
}
