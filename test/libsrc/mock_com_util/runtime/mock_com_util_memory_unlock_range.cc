#include <testfw.h>
#include <mock_com_util.h>

com_util_memory_lock_result_t delegate_real_com_util_memory_unlock_range(const void *address, size_t size)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_memory_unlock_range)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_memory_unlock_range"));

    return real_fn(address, size);
}

MOCK_WEAK_IMPL(com_util_memory_lock_result_t, com_util_memory_unlock_range, const void *address, size_t size)
{
    com_util_memory_lock_result_t rtc = COM_UTIL_MEMORY_LOCK_SYSTEM_ERROR;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_memory_unlock_range(address, size);
    }
    else
    {
        rtc = delegate_real_com_util_memory_unlock_range(address, size);
    }

    return rtc;
}
