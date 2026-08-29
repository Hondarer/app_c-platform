#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_free(void *ptr)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_free)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_free"));

    real_fn(ptr);
}

MOCK_WEAK_IMPL(void, cplat_free, void *ptr)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_free(ptr);
    }
    else
    {
        delegate_real_cplat_free(ptr);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p\n", __func__, ptr);
    }
}
