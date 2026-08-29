#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_hashtable_dispose(cplat_hashtable *ht)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_hashtable_dispose)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_hashtable_dispose"));

    real_fn(ht);
}

MOCK_WEAK_IMPL(void, cplat_hashtable_dispose, cplat_hashtable *ht)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_hashtable_dispose(ht);
    }
    else
    {
        delegate_real_cplat_hashtable_dispose(ht);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
