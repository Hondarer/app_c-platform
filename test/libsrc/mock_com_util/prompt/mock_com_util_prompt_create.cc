#include <testfw.h>
#include <mock_com_util.h>

WEAK_ATR com_util_prompt_t *com_util_prompt_create(size_t history_max)
{
    com_util_prompt_t *rtc = nullptr;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_prompt_create(history_max);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %zu", __func__, history_max);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}
