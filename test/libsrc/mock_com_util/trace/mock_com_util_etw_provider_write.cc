#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

int delegate_real_com_util_etw_provider_write(com_util_etw_provider_t *handle, int level, const char *service, const char *message)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_etw_provider_write)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_etw_provider_write"));

    return real_fn(handle, level, service, message);
}

MOCK_WEAK_IMPL(int, com_util_etw_provider_write, com_util_etw_provider_t *handle, int level, const char *service, const char *message)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_etw_provider_write(handle, level, service, message);
    }
    else
    {
        rtc = delegate_real_com_util_etw_provider_write(handle, level, service, message);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %d \"%s\"", __func__, level, message != nullptr ? message : "(null)");
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}

#endif /* PLATFORM_WINDOWS */
