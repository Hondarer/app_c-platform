#include <testfw.h>
#include <mock_cplat.h>

uint64_t delegate_real_cplat_get_monotonic_ms(void)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_get_monotonic_ms)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_get_monotonic_ms"));

    return real_fn();
}

MOCK_WEAK_IMPL(uint64_t, cplat_get_monotonic_ms, void)
{
    uint64_t mock_ret = 0;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_get_monotonic_ms();
    }
    else
    {
        mock_ret = delegate_real_cplat_get_monotonic_ms();
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %llu\n", (unsigned long long)mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
