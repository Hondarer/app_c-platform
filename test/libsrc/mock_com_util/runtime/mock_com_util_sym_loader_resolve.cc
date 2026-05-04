#include <testfw.h>
#include <mock_com_util.h>

void *delegate_real_com_util_sym_loader_resolve(com_util_sym_loader_entry_t *fobj)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_sym_loader_resolve)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_sym_loader_resolve"));

    return real_fn(fobj);
}

MOCK_WEAK_IMPL(void *, com_util_sym_loader_resolve, com_util_sym_loader_entry_t *fobj)
{
    void *rtc = nullptr;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_sym_loader_resolve(fobj);
    }
    else
    {
        delegate_real_com_util_sym_loader_resolve(fobj);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (void *)fobj);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}
