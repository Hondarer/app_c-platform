#include <testfw.h>
#include <mock_com_util.h>

WEAK_ATR void com_util_prompt_dispose(com_util_prompt_t *prompt)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_prompt_dispose(prompt);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p\n", __func__, (void *)prompt);
    }
}
