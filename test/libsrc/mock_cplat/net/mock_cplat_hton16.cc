#include <testfw.h>
#include <mock_cplat.h>

uint16_t delegate_real_cplat_hton16(uint16_t value)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_hton16)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_hton16"));

    return real_fn(value);
}

MOCK_WEAK_IMPL(uint16_t, cplat_hton16, uint16_t value)
{
    uint16_t mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_hton16(value);
    }
    else
    {
        mock_ret = delegate_real_cplat_hton16(value);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%x -> 0x%x\n", __func__, (unsigned int)value, (unsigned int)mock_ret);
    }

    return mock_ret;
}
