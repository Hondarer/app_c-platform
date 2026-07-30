#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_secure_zero(void *buf, size_t size)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_secure_zero)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_secure_zero"));

    real_fn(buf, size);
}

MOCK_WEAK_IMPL(void, com_util_secure_zero, void *buf, size_t size)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_secure_zero(buf, size);
    }
    else
    {
        delegate_real_com_util_secure_zero(buf, size);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p size=%zu\n", __func__, buf, size);
    }
}
