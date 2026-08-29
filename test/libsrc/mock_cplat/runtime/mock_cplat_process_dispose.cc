#include <testfw.h>
#include <mock_cplat.h>

void delegate_real_cplat_process_dispose(cplat_process *process)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_process_dispose)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_process_dispose"));

    real_fn(process);
}

MOCK_WEAK_IMPL(void, cplat_process_dispose, cplat_process *process)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_process_dispose(process);
    }
    else
    {
        delegate_real_cplat_process_dispose(process);
    }
}
