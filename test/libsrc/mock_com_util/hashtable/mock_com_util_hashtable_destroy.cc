#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_hashtable_destroy(com_util_hashtable *ht)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_hashtable_destroy)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_hashtable_destroy"));

    real_fn(ht);
}

MOCK_WEAK_IMPL(void, com_util_hashtable_destroy, com_util_hashtable *ht)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_hashtable_destroy(ht);
    }
    else
    {
        delegate_real_com_util_hashtable_destroy(ht);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
