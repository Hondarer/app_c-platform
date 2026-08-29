#include <testfw.h>
#include <mock_cplat.h>

uint32_t delegate_real_cplat_ntoh32(uint32_t value)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_ntoh32)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_ntoh32"));

    return real_fn(value);
}

MOCK_WEAK_IMPL(uint32_t, cplat_ntoh32, uint32_t value)
{
    uint32_t mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_ntoh32(value);
    }
    else
    {
        mock_ret = delegate_real_cplat_ntoh32(value);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%x -> 0x%x\n", __func__, (unsigned int)value, (unsigned int)mock_ret);
    }

    return mock_ret;
}
