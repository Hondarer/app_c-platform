#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_memory_lock_scope_release(com_util_memory_lock_scope *scope)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_memory_lock_scope_release)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_memory_lock_scope_release"));

    return real_fn(scope);
}

MOCK_WEAK_IMPL(int, com_util_memory_lock_scope_release, com_util_memory_lock_scope *scope)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_memory_lock_scope_release(scope);
    }
    else
    {
        mock_ret = delegate_real_com_util_memory_lock_scope_release(scope);
    }

    return mock_ret;
}
