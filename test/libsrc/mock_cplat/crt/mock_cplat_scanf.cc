#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_scanf(const char *format, va_list args)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_vscanf)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_vscanf"));

    return real_fn(format, args);
}

MOCK_WEAK_IMPL(int, cplat_scanf, const char *format, ...)
{
    int mock_ret = 0;
    va_list args;

    va_start(args, format);

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_scanf(format, args);
    }
    else
    {
        mock_ret = delegate_real_cplat_scanf(format, args);
    }

    va_end(args);

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
