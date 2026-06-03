#include <stdarg.h>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_pinned_prompt_printf(com_util_pinned_prompt *screen,
                                                com_util_pinned_prompt_channel_t channel, const char *fmt, ...)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_pinned_prompt_printf)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_pinned_prompt_printf"));

    return real_fn(screen, channel, "%s", fmt != nullptr ? fmt : "");
}

MOCK_WEAK_IMPL(int, com_util_pinned_prompt_printf, com_util_pinned_prompt *screen,
               com_util_pinned_prompt_channel_t channel, const char *fmt, ...)
{
    int rtc = -1;
    char buf[COM_UTIL_PROMPT_INPUT_BYTES_DEFAULT];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt != nullptr ? fmt : "", args);
    va_end(args);

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_pinned_prompt_printf(screen, channel, buf);
    }
    else
    {
        rtc = delegate_real_com_util_pinned_prompt_printf(screen, channel, buf);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %d, %s", __func__, (void *)screen, (int)channel, buf);
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
