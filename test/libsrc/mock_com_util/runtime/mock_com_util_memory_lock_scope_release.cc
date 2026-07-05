#include <testfw.h>
#include <mock_com_util.h>

com_util_memory_lock_result_t delegate_real_com_util_memory_lock_scope_release(com_util_memory_lock_scope *scope)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_memory_lock_scope_release)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_memory_lock_scope_release"));

    return real_fn(scope);
}

MOCK_WEAK_IMPL(com_util_memory_lock_result_t, com_util_memory_lock_scope_release, com_util_memory_lock_scope *scope)
{
    com_util_memory_lock_result_t rtc = COM_UTIL_MEMORY_LOCK_SYSTEM_ERROR;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_memory_lock_scope_release(scope);
    }
    else
    {
        rtc = delegate_real_com_util_memory_lock_scope_release(scope);
    }

    return rtc;
}
