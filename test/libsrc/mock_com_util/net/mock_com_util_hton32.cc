#include <testfw.h>
#include <mock_com_util.h>

uint32_t delegate_real_com_util_hton32(uint32_t value)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_hton32)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_hton32"));

    return real_fn(value);
}

MOCK_WEAK_IMPL(uint32_t, com_util_hton32, uint32_t value)
{
    uint32_t mock_ret;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_hton32(value);
    }
    else
    {
        mock_ret = delegate_real_com_util_hton32(value);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%x -> 0x%x\n", __func__, (unsigned int)value, (unsigned int)mock_ret);
    }

    return mock_ret;
}
