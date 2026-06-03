#include <testfw.h>
#include <mock_com_util.h>

size_t delegate_real_com_util_pinned_prompt_write(com_util_pinned_prompt *screen,
                                                  com_util_pinned_prompt_channel_t channel, const void *data,
                                                  size_t size)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_pinned_prompt_write)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_pinned_prompt_write"));

    return real_fn(screen, channel, data, size);
}

MOCK_WEAK_IMPL(size_t, com_util_pinned_prompt_write, com_util_pinned_prompt *screen,
               com_util_pinned_prompt_channel_t channel, const void *data, size_t size)
{
    size_t rtc = 0U;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_pinned_prompt_write(screen, channel, data, size);
    }
    else
    {
        rtc = delegate_real_com_util_pinned_prompt_write(screen, channel, data, size);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %d, 0x%p, %zu", __func__, (void *)screen, (int)channel, data, size);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %zu\n", rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}
