#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

int delegate_real_com_util_eventlog_register_source(const char *source_name, const char *message_file_path)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_eventlog_register_source)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_eventlog_register_source"));

    return real_fn(source_name, message_file_path);
}

MOCK_WEAK_IMPL(int, com_util_eventlog_register_source, const char *source_name, const char *message_file_path)
{
    int mock_ret = -1;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_eventlog_register_source(source_name, message_file_path);
    }
    else
    {
        mock_ret = delegate_real_com_util_eventlog_register_source(source_name, message_file_path);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s \"%s\" \"%s\"", __func__, source_name != nullptr ? source_name : "(null)",
               message_file_path != nullptr ? message_file_path : "(null)");
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
