#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_hashtable_add(com_util_hashtable *ht, const void *key, const void *value)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_hashtable_add)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_hashtable_add"));

    return real_fn(ht, key, value);
}

MOCK_WEAK_IMPL(int, com_util_hashtable_add, com_util_hashtable *ht, const void *key, const void *value)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_hashtable_add(ht, key, value);
    }
    else
    {
        mock_ret = delegate_real_com_util_hashtable_add(ht, key, value);
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
