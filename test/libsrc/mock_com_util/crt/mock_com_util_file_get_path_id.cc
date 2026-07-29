#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_file_get_path_id(const char *path, com_util_file_id *id_out)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_file_get_path_id)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_file_get_path_id"));

    return real_fn(path, id_out);
}

MOCK_WEAK_IMPL(int, com_util_file_get_path_id, const char *path, com_util_file_id *id_out)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_file_get_path_id(path, id_out);
    }
    else
    {
        rtc = delegate_real_com_util_file_get_path_id(path, id_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        const char *path_text = "(null)";
        if (path != nullptr)
        {
            path_text = path;
        }
        printf("  > %s \"%s\", 0x%p", __func__, path_text, (void *)id_out);
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
