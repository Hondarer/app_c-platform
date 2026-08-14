#include <testfw.h>
#include <mock_com_util.h>

const char *delegate_real_com_util_tracer_hex_sep(const char *message)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_tracer_hex_sep)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_tracer_hex_sep"));

    return real_fn(message);
}

MOCK_WEAK_IMPL(const char *, com_util_tracer_hex_sep, const char *message)
{
    const char *mock_ret;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_tracer_hex_sep(message);
    }
    else
    {
        mock_ret = delegate_real_com_util_tracer_hex_sep(message);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s -> %s\n", __func__, message == NULL ? "(null)" : message, mock_ret == NULL ? "(null)" : mock_ret);
    }

    return mock_ret;
}
