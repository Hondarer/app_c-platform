#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_free(void *ptr)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_free)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_free"));

    real_fn(ptr);
}

MOCK_WEAK_IMPL(void, com_util_free, void *ptr)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_free(ptr);
    }
    else
    {
        delegate_real_com_util_free(ptr);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p\n", __func__, ptr);
    }
}
