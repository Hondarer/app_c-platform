#include <stdarg.h>
#include <vector>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_tracer_writef_at(com_util_tracer *handle, com_util_trace_level level,
                                          const com_util_timespec *timestamp, const char *format, ...)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_tracer_writef_at)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_tracer_writef_at"));

    return real_fn(handle, level, timestamp, "%s", format);
}

MOCK_WEAK_IMPL(int, com_util_tracer_writef_at, com_util_tracer *handle, com_util_trace_level level,
               const com_util_timespec *timestamp, const char *format, ...)
{
    int rtc = 0;

    std::vector<char> buf;
    {
        va_list args;
        va_start(args, format);
        buf = mock_com_util_expand_format(format, args);
        va_end(args);
    }

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_tracer_writef_at(handle, level, timestamp, buf.data());
    }
    else
    {
        rtc = delegate_real_com_util_tracer_writef_at(handle, level, timestamp, buf.data());
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %d, 0x%p, %s", __func__, (void *)handle, (int)level, (const void *)timestamp, buf.data());
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
