#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_prompt_readline_at(com_util_prompt *p, char *buf, size_t buf_size, const char *prompt_str,
                                              const char *file, int line)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_prompt_readline_at)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_prompt_readline_at"));

    return real_fn(p, buf, buf_size, prompt_str, file, line);
}

MOCK_WEAK_IMPL(int, com_util_prompt_readline_at, com_util_prompt *p, char *buf, size_t buf_size, const char *prompt_str,
               const char *file, int line)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_prompt_readline_at(p, buf, buf_size, prompt_str, file, line);
    }
    else
    {
        rtc = delegate_real_com_util_prompt_readline_at(p, buf, buf_size, prompt_str, file, line);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s \"%s\"", __func__, prompt_str != nullptr ? prompt_str : "");
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
