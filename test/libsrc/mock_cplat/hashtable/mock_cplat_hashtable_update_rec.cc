#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_hashtable_update_rec(cplat_hashtable *ht, uint64_t record, const void *value)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_hashtable_update_rec)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_hashtable_update_rec"));

    return real_fn(ht, record, value);
}

MOCK_WEAK_IMPL(int, cplat_hashtable_update_rec, cplat_hashtable *ht, uint64_t record, const void *value)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_hashtable_update_rec(ht, record, value);
    }
    else
    {
        mock_ret = delegate_real_cplat_hashtable_update_rec(ht, record, value);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
