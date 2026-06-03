#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real__com_util_pinned_prompt_readline_fmt(com_util_pinned_prompt *screen, char *buf, size_t buf_size,
                                                       const char *file, int line, const char *fmt, va_list args)
{
    char prompt[COM_UTIL_PROMPT_INPUT_BYTES_DEFAULT];
    vsnprintf(prompt, sizeof(prompt), fmt != nullptr ? fmt : "", args);

    static auto real_fn = reinterpret_cast<decltype(&_com_util_pinned_prompt_readline)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "_com_util_pinned_prompt_readline"));

    return real_fn(screen, buf, buf_size, prompt, file, line);
}

MOCK_WEAK_IMPL(int, _com_util_pinned_prompt_readline_fmt, com_util_pinned_prompt *screen, char *buf, size_t buf_size,
               const char *file, int line, const char *fmt, ...)
{
    int rtc = 0;
    va_list args;

    va_start(args, fmt);

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->_com_util_pinned_prompt_readline_fmt(screen, buf, buf_size, file, line, fmt, args);
    }
    else
    {
        rtc = delegate_real__com_util_pinned_prompt_readline_fmt(screen, buf, buf_size, file, line, fmt, args);
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
