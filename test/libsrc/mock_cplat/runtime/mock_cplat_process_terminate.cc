#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_process_terminate(cplat_process *process)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_process_terminate)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_process_terminate"));

    return real_fn(process);
}

MOCK_WEAK_IMPL(int, cplat_process_terminate, cplat_process *process)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_process_terminate(process);
    }
    else
    {
        mock_ret = delegate_real_cplat_process_terminate(process);
    }

    return mock_ret;
}
