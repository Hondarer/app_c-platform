#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_vscanf(const char *format, va_list args)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_vscanf)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_vscanf"));

    return real_fn(format, args);
}

MOCK_WEAK_IMPL(int, com_util_vscanf, const char *format, va_list args)
{
    int mock_ret = 0;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_vscanf(format, args);
    }
    else
    {
        mock_ret = delegate_real_com_util_vscanf(format, args);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s \"%s\"", __func__, format);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
