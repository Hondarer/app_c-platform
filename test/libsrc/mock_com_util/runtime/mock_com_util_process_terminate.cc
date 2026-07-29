#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_process_terminate(com_util_process *process)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_process_terminate)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_process_terminate"));

    return real_fn(process);
}

MOCK_WEAK_IMPL(int, com_util_process_terminate, com_util_process *process)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_process_terminate(process);
    }
    else
    {
        rtc = delegate_real_com_util_process_terminate(process);
    }

    return rtc;
}
