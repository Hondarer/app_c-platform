#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_sym_loader_is_default(com_util_sym_loader_entry *fobj)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_sym_loader_is_default)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_sym_loader_is_default"));

    return real_fn(fobj);
}

MOCK_WEAK_IMPL(int, com_util_sym_loader_is_default, com_util_sym_loader_entry *fobj)
{
    int mock_ret = 0;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_sym_loader_is_default(fobj);
    }
    else
    {
        mock_ret = delegate_real_com_util_sym_loader_is_default(fobj);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (void *)fobj);
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
