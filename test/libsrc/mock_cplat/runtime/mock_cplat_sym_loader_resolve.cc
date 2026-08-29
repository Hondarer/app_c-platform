#include <testfw.h>
#include <mock_cplat.h>

void *delegate_real_cplat_sym_loader_resolve(cplat_sym_loader_entry *fobj)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_sym_loader_resolve)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_sym_loader_resolve"));

    return real_fn(fobj);
}

MOCK_WEAK_IMPL(void *, cplat_sym_loader_resolve, cplat_sym_loader_entry *fobj)
{
    void *mock_ret = nullptr;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_sym_loader_resolve(fobj);
    }
    else
    {
        delegate_real_cplat_sym_loader_resolve(fobj);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (void *)fobj);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
