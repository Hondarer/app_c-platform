#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_process_start(const cplat_process_options *options, cplat_process **process)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_process_start)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_process_start"));

    return real_fn(options, process);
}

MOCK_WEAK_IMPL(int, cplat_process_start, const cplat_process_options *options, cplat_process **process)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_process_start(options, process);
    }
    else
    {
        mock_ret = delegate_real_cplat_process_start(options, process);
    }

    return mock_ret;
}
