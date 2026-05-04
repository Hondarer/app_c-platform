#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

void delegate_real_com_util_etw_session_stop(com_util_etw_session_t *session)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_etw_session_stop)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_etw_session_stop"));

    real_fn(session);
}

MOCK_WEAK_IMPL(void, com_util_etw_session_stop, com_util_etw_session_t *session)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_etw_session_stop(session);
    }
    else
    {
        delegate_real_com_util_etw_session_stop(session);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p\n", __func__, (void *)session);
    }
}

#endif /* PLATFORM_WINDOWS */
