#include <stdarg.h>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real__com_util_tracer_writef(com_util_tracer *handle, com_util_trace_level_t level,
                                          const com_util_timespec *timestamp, const char *format, ...)
{
    static auto real_fn = reinterpret_cast<decltype(&_com_util_tracer_writef)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "_com_util_tracer_writef"));

    return real_fn(handle, level, timestamp, "%s", format);
}

MOCK_WEAK_IMPL(int, _com_util_tracer_writef, com_util_tracer *handle, com_util_trace_level_t level,
               const com_util_timespec *timestamp, const char *format, ...)
{
    int rtc = 0;

    char buf[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->_com_util_tracer_writef(handle, level, timestamp, buf);
    }
    else
    {
        rtc = delegate_real__com_util_tracer_writef(handle, level, timestamp, buf);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %d, 0x%p, %s", __func__, (void *)handle, (int)level, (const void *)timestamp, buf);
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
