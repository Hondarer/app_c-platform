#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_sym_loader_info(com_util_sym_loader_entry *const *fobj_array, size_t fobj_length)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_sym_loader_info)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_sym_loader_info"));

    return real_fn(fobj_array, fobj_length);
}

MOCK_WEAK_IMPL(int, com_util_sym_loader_info, com_util_sym_loader_entry *const *fobj_array, size_t fobj_length)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_sym_loader_info(fobj_array, fobj_length);
    }
    else
    {
        rtc = delegate_real_com_util_sym_loader_info(fobj_array, fobj_length);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
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
