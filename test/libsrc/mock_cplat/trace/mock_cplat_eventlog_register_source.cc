#include <testfw.h>
#include <mock_cplat.h>

#if defined(PLATFORM_WINDOWS)

int delegate_real_cplat_eventlog_register_source(const char *source_name, const char *message_file_path)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_eventlog_register_source)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_eventlog_register_source"));

    return real_fn(source_name, message_file_path);
}

MOCK_WEAK_IMPL(int, cplat_eventlog_register_source, const char *source_name, const char *message_file_path)
{
    int mock_ret = -1;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_eventlog_register_source(source_name, message_file_path);
    }
    else
    {
        mock_ret = delegate_real_cplat_eventlog_register_source(source_name, message_file_path);
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
