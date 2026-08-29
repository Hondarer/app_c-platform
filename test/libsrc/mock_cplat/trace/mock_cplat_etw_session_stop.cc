#include <testfw.h>
#include <mock_cplat.h>

#if defined(PLATFORM_WINDOWS)

void delegate_real_cplat_etw_session_stop(cplat_etw_session *session)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_etw_session_stop)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_etw_session_stop"));

    real_fn(session);
}

MOCK_WEAK_IMPL(void, cplat_etw_session_stop, cplat_etw_session *session)
{
    if (_mock_cplat != nullptr)
    {
        _mock_cplat->cplat_etw_session_stop(session);
    }
    else
    {
        delegate_real_cplat_etw_session_stop(session);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p\n", __func__, (void *)session);
    }
}

#endif /* PLATFORM_WINDOWS */
