#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_pinned_prompt_status_set(com_util_pinned_prompt_t *screen,
                                                    com_util_pinned_prompt_status_position_t position,
                                                    com_util_pinned_prompt_status_align_t align,
                                                    const char *content)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_pinned_prompt_status_set)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_pinned_prompt_status_set"));

    return real_fn(screen, position, align, content);
}

MOCK_WEAK_IMPL(int, com_util_pinned_prompt_status_set, com_util_pinned_prompt_t *screen,
               com_util_pinned_prompt_status_position_t position,
               com_util_pinned_prompt_status_align_t align,
               const char *content)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_pinned_prompt_status_set(screen, position, align, content);
    }
    else
    {
        rtc = delegate_real_com_util_pinned_prompt_status_set(screen, position, align, content);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %d, %d, \"%s\"", __func__, (void *)screen, (int)position, (int)align,
               content != nullptr ? content : "");
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
