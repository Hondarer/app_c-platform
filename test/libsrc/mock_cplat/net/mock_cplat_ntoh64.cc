#include <testfw.h>
#include <mock_cplat.h>

uint64_t delegate_real_cplat_ntoh64(uint64_t value)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_ntoh64)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_ntoh64"));

    return real_fn(value);
}

MOCK_WEAK_IMPL(uint64_t, cplat_ntoh64, uint64_t value)
{
    uint64_t mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_ntoh64(value);
    }
    else
    {
        mock_ret = delegate_real_cplat_ntoh64(value);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%llx -> 0x%llx\n", __func__, (unsigned long long)value, (unsigned long long)mock_ret);
    }

    return mock_ret;
}
