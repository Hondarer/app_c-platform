#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_hashtable_resize(cplat_hashtable *ht, const cplat_hashtable_config *new_config)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_hashtable_resize)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_hashtable_resize"));

    return real_fn(ht, new_config);
}

MOCK_WEAK_IMPL(int, cplat_hashtable_resize, cplat_hashtable *ht, const cplat_hashtable_config *new_config)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_hashtable_resize(ht, new_config);
    }
    else
    {
        mock_ret = delegate_real_cplat_hashtable_resize(ht, new_config);
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
