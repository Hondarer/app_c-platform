#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_hashtable_dispose(com_util_hashtable *ht)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_hashtable_dispose)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_hashtable_dispose"));

    real_fn(ht);
}

MOCK_WEAK_IMPL(void, com_util_hashtable_dispose, com_util_hashtable *ht)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_hashtable_dispose(ht);
    }
    else
    {
        delegate_real_com_util_hashtable_dispose(ht);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
