#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_sleep_ms(int ms)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_sleep_ms)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_sleep_ms"));

    real_fn(ms);
}

MOCK_WEAK_IMPL(void, cplat_sleep_ms, int ms)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_sleep_ms(ms);
    }
    else
    {
        delegate_real_cplat_sleep_ms(ms);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d\n", __func__, ms);
    }
}
