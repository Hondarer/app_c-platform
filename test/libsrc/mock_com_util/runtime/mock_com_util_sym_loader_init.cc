#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_sym_loader_init(com_util_sym_loader_entry_t *const *fobj_array, size_t fobj_length, const char *configpath)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_sym_loader_init)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_sym_loader_init"));

    real_fn(fobj_array, fobj_length, configpath);
}

WEAK_ATR void com_util_sym_loader_init(com_util_sym_loader_entry_t *const *fobj_array, size_t fobj_length, const char *configpath)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_sym_loader_init(fobj_array, fobj_length, configpath);
    }
    else
    {
        delegate_real_com_util_sym_loader_init(fobj_array, fobj_length, configpath);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s \"%s\"\n", __func__, configpath != nullptr ? configpath : "(null)");
    }
}
