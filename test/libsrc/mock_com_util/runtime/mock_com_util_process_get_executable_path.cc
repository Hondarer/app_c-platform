#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_process_get_executable_path(char *path_out, size_t path_size)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_process_get_executable_path)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_process_get_executable_path"));

    return real_fn(path_out, path_size);
}

MOCK_WEAK_IMPL(int, com_util_process_get_executable_path, char *path_out, size_t path_size)
{
    int mock_ret = -1;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_process_get_executable_path(path_out, path_size);
    }
    else
    {
        mock_ret = delegate_real_com_util_process_get_executable_path(path_out, path_size);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
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
