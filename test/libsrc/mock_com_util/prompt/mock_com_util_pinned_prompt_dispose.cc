#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_pinned_prompt_dispose(com_util_pinned_prompt_t *screen)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_pinned_prompt_dispose)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_pinned_prompt_dispose"));

    real_fn(screen);
}

MOCK_WEAK_IMPL(void, com_util_pinned_prompt_dispose, com_util_pinned_prompt_t *screen)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_pinned_prompt_dispose(screen);
    }
    else
    {
        delegate_real_com_util_pinned_prompt_dispose(screen);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p\n", __func__, (void *)screen);
    }
}
