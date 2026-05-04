#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_rewind(FILE *stream)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_rewind)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_rewind"));

    real_fn(stream);
}

MOCK_WEAK_IMPL(void, com_util_rewind, FILE *stream)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_rewind(stream);
    }
    else
    {
        delegate_real_com_util_rewind(stream);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p\n", __func__, (void *)stream);
    }
}
