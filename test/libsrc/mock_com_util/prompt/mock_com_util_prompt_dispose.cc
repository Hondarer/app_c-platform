#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_prompt_dispose(com_util_prompt *prompt)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_prompt_dispose)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_prompt_dispose"));

    real_fn(prompt);
}

MOCK_WEAK_IMPL(void, com_util_prompt_dispose, com_util_prompt *prompt)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_prompt_dispose(prompt);
    }
    else
    {
        delegate_real_com_util_prompt_dispose(prompt);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p\n", __func__, (void *)prompt);
    }
}
