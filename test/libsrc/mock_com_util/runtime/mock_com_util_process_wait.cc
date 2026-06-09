#include <testfw.h>
#include <mock_com_util.h>

com_util_process_result_t delegate_real_com_util_process_wait(com_util_process *process, int timeout_ms)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_process_wait)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_process_wait"));

    return real_fn(process, timeout_ms);
}

MOCK_WEAK_IMPL(com_util_process_result_t, com_util_process_wait, com_util_process *process, int timeout_ms)
{
    com_util_process_result_t rtc = COM_UTIL_PROCESS_SYSTEM_ERROR;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_process_wait(process, timeout_ms);
    }
    else
    {
        rtc = delegate_real_com_util_process_wait(process, timeout_ms);
    }

    return rtc;
}
