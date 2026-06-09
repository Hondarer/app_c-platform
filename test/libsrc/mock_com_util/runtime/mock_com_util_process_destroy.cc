#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_process_destroy(com_util_process *process)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_process_destroy)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_process_destroy"));

    real_fn(process);
}

MOCK_WEAK_IMPL(void, com_util_process_destroy, com_util_process *process)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_process_destroy(process);
    }
    else
    {
        delegate_real_com_util_process_destroy(process);
    }
}
