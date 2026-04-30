#include <testfw.h>
#include <mock_com_util.h>

WEAK_ATR int _com_util_prompt_readline_fmt(com_util_prompt_t *p, char *buf, size_t buf_size,
                                            const char *file, int line, const char *fmt, ...)
{
    int rtc = 0;
    va_list args;

    va_start(args, fmt);

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->_com_util_prompt_readline_fmt(p, buf, buf_size, file, line, fmt, args);
    }

    va_end(args);

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s \"%s\"", __func__, fmt != nullptr ? fmt : "");
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
