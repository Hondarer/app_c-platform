#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_vscanf(const char *format, va_list args)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_vscanf)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_vscanf"));

    return real_fn(format, args);
}

MOCK_WEAK_IMPL(int, cplat_vscanf, const char *format, va_list args)
{
    int mock_ret = 0;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_vscanf(format, args);
    }
    else
    {
        mock_ret = delegate_real_cplat_vscanf(format, args);
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
