#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_console_write(cplat_stream stream, const char *text)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_console_write)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_console_write"));

    return real_fn(stream, text);
}

MOCK_WEAK_IMPL(int, cplat_console_write, cplat_stream stream, const char *text)
{
    int mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_console_write(stream, text);
    }
    else
    {
        mock_ret = delegate_real_cplat_console_write(stream, text);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }

    return mock_ret;
}
