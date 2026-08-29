#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_file_set_size(cplat_file *file, size_t size, cplat_error *detail_out)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_file_set_size)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_file_set_size"));

    return real_fn(file, size, detail_out);
}

MOCK_WEAK_IMPL(int, cplat_file_set_size, cplat_file *file, size_t size, cplat_error *detail_out)
{
    if (_mock_cplat != nullptr)
    {
        return _mock_cplat->cplat_file_set_size(file, size, detail_out);
    }

    return delegate_real_cplat_file_set_size(file, size, detail_out);
}
