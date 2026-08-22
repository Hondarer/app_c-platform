#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_hashtable_insert_direct(com_util_hashtable *ht, uint64_t record, const void *key, int status,
                                                   const void *value, const com_util_timespec *timestamp,
                                                   uint64_t generation)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_hashtable_insert_direct)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_hashtable_insert_direct"));

    return real_fn(ht, record, key, status, value, timestamp, generation);
}

MOCK_WEAK_IMPL(int, com_util_hashtable_insert_direct, com_util_hashtable *ht, uint64_t record, const void *key,
               int status, const void *value, const com_util_timespec *timestamp, uint64_t generation)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret =
            _mock_com_util->com_util_hashtable_insert_direct(ht, record, key, status, value, timestamp, generation);
    }
    else
    {
        mock_ret = delegate_real_com_util_hashtable_insert_direct(ht, record, key, status, value, timestamp,
                                                                   generation);
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
