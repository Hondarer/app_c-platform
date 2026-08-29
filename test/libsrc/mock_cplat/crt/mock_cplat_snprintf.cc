#include <stdarg.h>
#include <stdio.h>
#include <vector>
#include <testfw.h>
#include <mock_cplat.h>

// mock_cplat_vsnprintf.cc で定義する、書式を切り詰めずに展開するヘルパー
extern std::vector<char> mock_cplat_expand_format(const char *format, va_list args);

int delegate_real_cplat_snprintf(char *dest, size_t dest_size, const char *format, ...)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_snprintf)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_snprintf"));

    return real_fn(dest, dest_size, "%s", format);
}

MOCK_WEAK_IMPL(int, cplat_snprintf, char *dest, size_t dest_size, const char *format, ...)
{
    int mock_ret = -1;
    std::vector<char> buf;

    {
        va_list args;
        va_start(args, format);
        buf = mock_cplat_expand_format(format, args);
        va_end(args);
    }

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_snprintf(dest, dest_size, buf.data());
    }
    else
    {
        mock_ret = delegate_real_cplat_snprintf(dest, dest_size, buf.data());
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu, %s", __func__, (void *)dest, dest_size, buf.data());
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
