#include <stdarg.h>
#include <vector>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_fprintf(FILE *stream, const char *format, ...)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_fprintf)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_fprintf"));

    return real_fn(stream, "%s", format);
}

MOCK_WEAK_IMPL(int, com_util_fprintf, FILE *stream, const char *format, ...)
{
    int mock_ret = -1;

    std::vector<char> buf;
    {
        va_list args;
        va_start(args, format);
        buf = mock_com_util_expand_format(format, args);
        va_end(args);
    }

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_fprintf(stream, buf.data());
    }
    else
    {
        mock_ret = delegate_real_com_util_fprintf(stream, buf.data());
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %s", __func__, (void *)stream, buf.data());
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
