#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_pinned_prompt_status_enable(com_util_pinned_prompt *screen,
                                                       com_util_pinned_prompt_status_position position, int enable)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_pinned_prompt_status_enable)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_pinned_prompt_status_enable"));

    return real_fn(screen, position, enable);
}

MOCK_WEAK_IMPL(int, com_util_pinned_prompt_status_enable, com_util_pinned_prompt *screen,
               com_util_pinned_prompt_status_position position, int enable)
{
    int mock_ret = -1;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_pinned_prompt_status_enable(screen, position, enable);
    }
    else
    {
        mock_ret = delegate_real_com_util_pinned_prompt_status_enable(screen, position, enable);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %d, %d", __func__, (void *)screen, (int)position, enable);
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
