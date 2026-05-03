#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_clearerr(FILE *stream)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_clearerr)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_clearerr"));

    real_fn(stream);
}

WEAK_ATR void com_util_clearerr(FILE *stream)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_clearerr(stream);
    }
    else
    {
        delegate_real_com_util_clearerr(stream);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p\n", __func__, (void *)stream);
    }
}
