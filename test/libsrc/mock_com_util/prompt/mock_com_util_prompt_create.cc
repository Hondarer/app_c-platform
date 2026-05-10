#include <testfw.h>
#include <mock_com_util.h>

com_util_prompt_t *delegate_real_com_util_prompt_create(const com_util_prompt_options_t *options)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_prompt_create)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_prompt_create"));

    return real_fn(options);
}

MOCK_WEAK_IMPL(com_util_prompt_t *, com_util_prompt_create, const com_util_prompt_options_t *options)
{
    com_util_prompt_t *rtc = nullptr;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_prompt_create(options);
    }
    else
    {
        rtc = delegate_real_com_util_prompt_create(options);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (const void *)options);
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
