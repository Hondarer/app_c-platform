#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_sym_loader_dispose(com_util_sym_loader_entry_t *const *fobj_array, size_t fobj_length)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_sym_loader_dispose)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_sym_loader_dispose"));

    real_fn(fobj_array, fobj_length);
}

WEAK_ATR void com_util_sym_loader_dispose(com_util_sym_loader_entry_t *const *fobj_array, size_t fobj_length)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_sym_loader_dispose(fobj_array, fobj_length);
    }
    else
    {
        delegate_real_com_util_sym_loader_dispose(fobj_array, fobj_length);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
