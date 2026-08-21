#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_process_dispose(com_util_process *process)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_process_dispose)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_process_dispose"));

    real_fn(process);
}

MOCK_WEAK_IMPL(void, com_util_process_dispose, com_util_process *process)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_process_dispose(process);
    }
    else
    {
        delegate_real_com_util_process_dispose(process);
    }
}
