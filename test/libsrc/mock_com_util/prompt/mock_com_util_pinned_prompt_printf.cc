#include <stdarg.h>
#include <vector>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_pinned_prompt_printf(com_util_pinned_prompt *screen, com_util_pinned_prompt_channel channel,
                                                const char *fmt, ...)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_pinned_prompt_printf)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_pinned_prompt_printf"));

    if (fmt == nullptr)
    {
        return real_fn(screen, channel, "%s", "");
    }

    return real_fn(screen, channel, "%s", fmt);
}

MOCK_WEAK_IMPL(int, com_util_pinned_prompt_printf, com_util_pinned_prompt *screen,
               com_util_pinned_prompt_channel channel, const char *fmt, ...)
{
    int mock_ret = -1;
    std::vector<char> buf;

    {
        va_list args;
        va_start(args, fmt);
        /* fmt が NULL の場合は空文字列として展開される */
        buf = mock_com_util_expand_format(fmt, args);
        va_end(args);
    }

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_pinned_prompt_printf(screen, channel, buf.data());
    }
    else
    {
        mock_ret = delegate_real_com_util_pinned_prompt_printf(screen, channel, buf.data());
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %d, %s", __func__, (void *)screen, (int)channel, buf.data());
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
