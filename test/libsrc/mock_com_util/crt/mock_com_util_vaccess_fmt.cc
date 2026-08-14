#include <stdarg.h>
#include <vector>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_vaccess_fmt(int mode, com_util_error *detail_out, const char *format, va_list args)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_vaccess_fmt)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_vaccess_fmt"));

    return real_fn(mode, detail_out, format, args);
}

MOCK_WEAK_IMPL(int, com_util_vaccess_fmt, int mode, com_util_error *detail_out, const char *format, va_list args)
{
    int mock_ret = -1;

    std::vector<char> buf = mock_com_util_expand_format(format, args);

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_vaccess_fmt(mode, detail_out, buf.data());
    }
    else
    {
        mock_ret = delegate_real_com_util_vaccess_fmt(mode, detail_out, format, args);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d, %s", __func__, mode, buf.data());
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
