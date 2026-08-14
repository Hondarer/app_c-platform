#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_process_wait(com_util_process *process, int timeout_ms)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_process_wait)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_process_wait"));

    return real_fn(process, timeout_ms);
}

MOCK_WEAK_IMPL(int, com_util_process_wait, com_util_process *process, int timeout_ms)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_process_wait(process, timeout_ms);
    }
    else
    {
        mock_ret = delegate_real_com_util_process_wait(process, timeout_ms);
    }

    return mock_ret;
}
