#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_memory_lock_scope_release(cplat_memory_lock_scope *scope)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_memory_lock_scope_release)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_memory_lock_scope_release"));

    return real_fn(scope);
}

MOCK_WEAK_IMPL(int, cplat_memory_lock_scope_release, cplat_memory_lock_scope *scope)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_memory_lock_scope_release(scope);
    }
    else
    {
        mock_ret = delegate_real_cplat_memory_lock_scope_release(scope);
    }

    return mock_ret;
}
