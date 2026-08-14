#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_memory_lock_range(const void *address, size_t size)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_memory_lock_range)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_memory_lock_range"));

    return real_fn(address, size);
}

MOCK_WEAK_IMPL(int, com_util_memory_lock_range, const void *address, size_t size)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_memory_lock_range(address, size);
    }
    else
    {
        mock_ret = delegate_real_com_util_memory_lock_range(address, size);
    }

    return mock_ret;
}
