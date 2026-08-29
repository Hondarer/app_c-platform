#include <testfw.h>
#include <mock_cplat.h>

cplat_prompt *delegate_real_cplat_prompt_create(const cplat_prompt_options *options)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_prompt_create)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_prompt_create"));

    return real_fn(options);
}

MOCK_WEAK_IMPL(cplat_prompt *, cplat_prompt_create, const cplat_prompt_options *options)
{
    cplat_prompt *mock_ret = nullptr;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_prompt_create(options);
    }
    else
    {
        mock_ret = delegate_real_cplat_prompt_create(options);
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
