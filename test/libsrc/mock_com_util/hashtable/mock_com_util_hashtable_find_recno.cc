#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_hashtable_find_recno(const com_util_hashtable *ht, const void *key, uint64_t *record_out)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_hashtable_find_recno)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_hashtable_find_recno"));

    return real_fn(ht, key, record_out);
}

MOCK_WEAK_IMPL(int, com_util_hashtable_find_recno, const com_util_hashtable *ht, const void *key, uint64_t *record_out)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_hashtable_find_recno(ht, key, record_out);
    }
    else
    {
        mock_ret = delegate_real_com_util_hashtable_find_recno(ht, key, record_out);
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
