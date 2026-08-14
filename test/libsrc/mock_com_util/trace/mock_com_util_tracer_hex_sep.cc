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
    const char *result;

    if (_mock_com_util != nullptr)
    {
        result = _mock_com_util->com_util_tracer_hex_sep(message);
    }
    else
    {
        result = delegate_real_com_util_tracer_hex_sep(message);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s -> %s\n", __func__, message == NULL ? "(null)" : message, result == NULL ? "(null)" : result);
    }

    return result;
}
