#include <stdarg.h>
#include <vector>
#include <stdio.h>
#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_vfprintf(FILE *stream, const char *format, va_list args)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_vfprintf)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_vfprintf"));

    return real_fn(stream, format, args);
}

MOCK_WEAK_IMPL(int, cplat_vfprintf, FILE *stream, const char *format, va_list args)
{
    int mock_ret = -1;

    std::vector<char> buf = mock_cplat_expand_format(format, args);

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_vfprintf(stream, buf.data());
    }
    else
    {
        mock_ret = delegate_real_cplat_vfprintf(stream, format, args);
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
