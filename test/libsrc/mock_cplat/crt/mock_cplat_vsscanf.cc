#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_vsscanf(const char *buffer, const char *format, va_list args)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_vsscanf)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_vsscanf"));

    return real_fn(buffer, format, args);
}

MOCK_WEAK_IMPL(int, cplat_vsscanf, const char *buffer, const char *format, va_list args)
{
    int mock_ret = 0;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_vsscanf(buffer, format, args);
    }
    else
    {
        mock_ret = delegate_real_cplat_vsscanf(buffer, format, args);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s \"%s\", \"%s\"", __func__, buffer, format);
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
