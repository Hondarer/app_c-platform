#include <testfw.h>
#include <mock_com_util.h>

WEAK_ATR int _com_util_prompt_readline(com_util_prompt_t *p, char *buf, size_t buf_size,
                                        const char *prompt_str, const char *file, int line)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->_com_util_prompt_readline(p, buf, buf_size, prompt_str, file, line);
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
