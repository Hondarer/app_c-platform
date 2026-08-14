#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_trace_file_sink_write(com_util_trace_file_sink *handle, int level,
                                                 const com_util_timespec *timestamp, const char *message)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_trace_file_sink_write)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_trace_file_sink_write"));

    return real_fn(handle, level, timestamp, message);
}

MOCK_WEAK_IMPL(int, com_util_trace_file_sink_write, com_util_trace_file_sink *handle, int level,
               const com_util_timespec *timestamp, const char *message)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_trace_file_sink_write(handle, level, timestamp, message);
    }
    else
    {
        mock_ret = delegate_real_com_util_trace_file_sink_write(handle, level, timestamp, message);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        const char *message_text = "(null)";
        if (message != nullptr)
        {
            message_text = message;
        }
        printf("  > %s %d 0x%p \"%s\"", __func__, level, (const void *)timestamp, message_text);
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
