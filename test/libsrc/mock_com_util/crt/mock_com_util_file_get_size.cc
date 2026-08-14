#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_file_get_size(const com_util_file *file, size_t *size_out, com_util_error *detail_out)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_file_get_size)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_file_get_size"));

    return real_fn(file, size_out, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_file_get_size, const com_util_file *file, size_t *size_out, com_util_error *detail_out)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_file_get_size(file, size_out, detail_out);
    }
    else
    {
        mock_ret = delegate_real_com_util_file_get_size(file, size_out, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, 0x%p", __func__, (const void *)file, (void *)size_out);
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
