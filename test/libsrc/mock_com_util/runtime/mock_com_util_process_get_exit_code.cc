#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_process_get_exit_code(com_util_process *process, int *exit_code)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_process_get_exit_code)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_process_get_exit_code"));

    return real_fn(process, exit_code);
}

MOCK_WEAK_IMPL(int, com_util_process_get_exit_code, com_util_process *process, int *exit_code)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_process_get_exit_code(process, exit_code);
    }
    else
    {
        mock_ret = delegate_real_com_util_process_get_exit_code(process, exit_code);
    }

    return mock_ret;
}
