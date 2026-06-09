#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_file_get_id(const com_util_file *file, com_util_file_id *id_out)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_file_get_id)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_file_get_id"));

    return real_fn(file, id_out);
}

MOCK_WEAK_IMPL(int, com_util_file_get_id, const com_util_file *file, com_util_file_id *id_out)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_file_get_id(file, id_out);
    }
    else
    {
        rtc = delegate_real_com_util_file_get_id(file, id_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, 0x%p", __func__, (const void *)file, (void *)id_out);
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
