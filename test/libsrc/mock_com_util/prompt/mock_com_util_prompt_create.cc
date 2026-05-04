#include <testfw.h>
#include <mock_com_util.h>

com_util_prompt_t *delegate_real_com_util_prompt_create(size_t history_max)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_prompt_create)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_prompt_create"));

    return real_fn(history_max);
}

MOCK_WEAK_IMPL(com_util_prompt_t *, com_util_prompt_create, size_t history_max)
{
    com_util_prompt_t *rtc = nullptr;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_prompt_create(history_max);
    }
    else
    {
        rtc = delegate_real_com_util_prompt_create(history_max);
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
