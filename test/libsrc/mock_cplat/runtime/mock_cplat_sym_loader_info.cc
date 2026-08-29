#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_sym_loader_info(cplat_sym_loader_entry *const *fobj_array, size_t fobj_length)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_sym_loader_info)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_sym_loader_info"));

    return real_fn(fobj_array, fobj_length);
}

MOCK_WEAK_IMPL(int, cplat_sym_loader_info, cplat_sym_loader_entry *const *fobj_array, size_t fobj_length)
{
    int mock_ret = -1;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_sym_loader_info(fobj_array, fobj_length);
    }
    else
    {
        mock_ret = delegate_real_cplat_sym_loader_info(fobj_array, fobj_length);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
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
