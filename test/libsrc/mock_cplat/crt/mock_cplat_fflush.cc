#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_fflush(FILE *stream, cplat_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_fflush)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_fflush"));

    return real_fn(stream, detail_out);
}

MOCK_WEAK_IMPL(int, cplat_fflush, FILE *stream, cplat_error *detail_out)
{
    if (_mock_cplat != nullptr)
    {
        return _mock_cplat->cplat_fflush(stream, detail_out);
    }

    return delegate_real_cplat_fflush(stream, detail_out);
}
