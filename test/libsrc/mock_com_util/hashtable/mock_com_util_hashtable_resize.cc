#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_hashtable_resize(com_util_hashtable *ht, const com_util_hashtable_config *new_config)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_hashtable_resize)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_hashtable_resize"));

    return real_fn(ht, new_config);
}

MOCK_WEAK_IMPL(int, com_util_hashtable_resize, com_util_hashtable *ht, const com_util_hashtable_config *new_config)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_hashtable_resize(ht, new_config);
    }
    else
    {
        mock_ret = delegate_real_com_util_hashtable_resize(ht, new_config);
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
