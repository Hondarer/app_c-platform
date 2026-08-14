#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

int delegate_real_com_util_etw_provider_write(com_util_etw_provider *handle, int level, const char *service,
                                              const char *message)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_etw_provider_write)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_etw_provider_write"));

    return real_fn(handle, level, service, message);
}

MOCK_WEAK_IMPL(int, com_util_etw_provider_write, com_util_etw_provider *handle, int level, const char *service,
               const char *message)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_etw_provider_write(handle, level, service, message);
    }
    else
    {
        mock_ret = delegate_real_com_util_etw_provider_write(handle, level, service, message);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        const char *message_text = "(null)";
        if (message != nullptr)
        {
            message_text = message;
        }
        printf("  > %s %d \"%s\"", __func__, level, message_text);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}

#endif /* PLATFORM_WINDOWS */
