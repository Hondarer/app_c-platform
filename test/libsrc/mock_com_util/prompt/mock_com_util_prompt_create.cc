#include <testfw.h>
#include <mock_com_util.h>

com_util_prompt *delegate_real_com_util_prompt_create(const com_util_prompt_options *options)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_prompt_create)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_prompt_create"));

    return real_fn(options);
}

MOCK_WEAK_IMPL(com_util_prompt *, com_util_prompt_create, const com_util_prompt_options *options)
{
    com_util_prompt *mock_ret = nullptr;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_prompt_create(options);
    }
    else
    {
        mock_ret = delegate_real_com_util_prompt_create(options);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (const void *)options);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
