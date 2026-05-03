#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

com_util_etw_session_t *delegate_real_com_util_etw_session_start(
    const char *session_name,
    const char *provider_guid_str,
    com_util_etw_event_callback_t callback,
    void *context,
    int *out_status)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_etw_session_start)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_etw_session_start"));

    return real_fn(session_name, provider_guid_str, callback, context, out_status);
}

WEAK_ATR com_util_etw_session_t *com_util_etw_session_start(
    const char *session_name,
    const char *provider_guid_str,
    com_util_etw_event_callback_t callback,
    void *context,
    int *out_status)
{
    com_util_etw_session_t *rtc = nullptr;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_etw_session_start(
            session_name, provider_guid_str, callback, context, out_status);
    }
    else
    {
        rtc = delegate_real_com_util_etw_session_start(session_name, provider_guid_str, callback, context, out_status);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s \"%s\"", __func__, session_name != nullptr ? session_name : "(null)");
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}

#endif /* PLATFORM_WINDOWS */
