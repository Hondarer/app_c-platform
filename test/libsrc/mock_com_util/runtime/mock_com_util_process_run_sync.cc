#include <testfw.h>
#include <mock_com_util.h>

com_util_process_result_t delegate_real_com_util_process_run_sync(const com_util_process_options_t *options,
                                                                  int timeout_ms, int *exit_code)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_process_run_sync)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_process_run_sync"));

    return real_fn(options, timeout_ms, exit_code);
}

MOCK_WEAK_IMPL(com_util_process_result_t, com_util_process_run_sync, const com_util_process_options_t *options,
               int timeout_ms, int *exit_code)
{
    com_util_process_result_t rtc = COM_UTIL_PROCESS_SYSTEM_ERROR;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_process_run_sync(options, timeout_ms, exit_code);
    }
    else
    {
        rtc = delegate_real_com_util_process_run_sync(options, timeout_ms, exit_code);
    }

    return rtc;
}
