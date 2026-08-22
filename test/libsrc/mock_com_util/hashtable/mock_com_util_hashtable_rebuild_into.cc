#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_hashtable_rebuild_into(const com_util_hashtable *src,
                                                  const com_util_hashtable_config *new_config, void *buf_mgmt,
                                                  size_t buf_mgmt_size, void *buf_data, size_t buf_data_size,
                                                  com_util_hashtable **ht_out)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_hashtable_rebuild_into)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_hashtable_rebuild_into"));

    return real_fn(src, new_config, buf_mgmt, buf_mgmt_size, buf_data, buf_data_size, ht_out);
}

MOCK_WEAK_IMPL(int, com_util_hashtable_rebuild_into, const com_util_hashtable *src,
               const com_util_hashtable_config *new_config, void *buf_mgmt, size_t buf_mgmt_size, void *buf_data,
               size_t buf_data_size, com_util_hashtable **ht_out)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_hashtable_rebuild_into(src, new_config, buf_mgmt, buf_mgmt_size, buf_data,
                                                                   buf_data_size, ht_out);
    }
    else
    {
        mock_ret = delegate_real_com_util_hashtable_rebuild_into(src, new_config, buf_mgmt, buf_mgmt_size, buf_data,
                                                                 buf_data_size, ht_out);
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
