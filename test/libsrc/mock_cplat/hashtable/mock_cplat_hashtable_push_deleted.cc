#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_hashtable_push_deleted(cplat_hashtable *ht)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_hashtable_push_deleted)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_hashtable_push_deleted"));

    return real_fn(ht);
}

MOCK_WEAK_IMPL(int, cplat_hashtable_push_deleted, cplat_hashtable *ht)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_hashtable_push_deleted(ht);
    }
    else
    {
        mock_ret = delegate_real_cplat_hashtable_push_deleted(ht);
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
