#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_sym_loader_dispose(cplat_sym_loader_entry *const *fobj_array, size_t fobj_length)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_sym_loader_dispose)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_sym_loader_dispose"));

    real_fn(fobj_array, fobj_length);
}

MOCK_WEAK_IMPL(void, cplat_sym_loader_dispose, cplat_sym_loader_entry *const *fobj_array, size_t fobj_length)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_sym_loader_dispose(fobj_array, fobj_length);
    }
    else
    {
        delegate_real_cplat_sym_loader_dispose(fobj_array, fobj_length);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
