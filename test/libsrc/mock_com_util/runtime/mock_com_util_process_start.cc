#include <testfw.h>
#include <mock_com_util.h>

com_util_process_result_t delegate_real_com_util_process_start(const com_util_process_options_t *options,
                                                               com_util_process **process)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_process_start)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_process_start"));

    return real_fn(options, process);
}

MOCK_WEAK_IMPL(com_util_process_result_t, com_util_process_start, const com_util_process_options_t *options,
               com_util_process **process)
{
    com_util_process_result_t rtc = COM_UTIL_PROCESS_SYSTEM_ERROR;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_process_start(options, process);
    }
    else
    {
        rtc = delegate_real_com_util_process_start(options, process);
    }

    return rtc;
}
