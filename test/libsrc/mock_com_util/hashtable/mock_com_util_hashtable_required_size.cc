#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_hashtable_required_size(const com_util_hashtable_config *config, size_t *mgmt_size_out,
                                                   size_t *data_size_out)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_hashtable_required_size)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_hashtable_required_size"));

    return real_fn(config, mgmt_size_out, data_size_out);
}

MOCK_WEAK_IMPL(int, com_util_hashtable_required_size, const com_util_hashtable_config *config, size_t *mgmt_size_out,
               size_t *data_size_out)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_hashtable_required_size(config, mgmt_size_out, data_size_out);
    }
    else
    {
        mock_ret = delegate_real_com_util_hashtable_required_size(config, mgmt_size_out, data_size_out);
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
