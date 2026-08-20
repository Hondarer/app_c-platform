#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_hashtable_count_status(const com_util_hashtable *ht, size_t *in_use_out, size_t *deleted_out,
                                                  size_t *empty_out)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_hashtable_count_status)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_hashtable_count_status"));

    return real_fn(ht, in_use_out, deleted_out, empty_out);
}

MOCK_WEAK_IMPL(int, com_util_hashtable_count_status, const com_util_hashtable *ht, size_t *in_use_out,
               size_t *deleted_out, size_t *empty_out)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_hashtable_count_status(ht, in_use_out, deleted_out, empty_out);
    }
    else
    {
        mock_ret = delegate_real_com_util_hashtable_count_status(ht, in_use_out, deleted_out, empty_out);
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
