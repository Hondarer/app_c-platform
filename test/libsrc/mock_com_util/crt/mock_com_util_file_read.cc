#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_file_read(com_util_file *file, void *buf, size_t len, size_t *read_out,
                                     com_util_error *detail_out)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_file_read)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_file_read"));

    return real_fn(file, buf, len, read_out, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_file_read, com_util_file *file, void *buf, size_t len, size_t *read_out,
               com_util_error *detail_out)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_file_read(file, buf, len, read_out, detail_out);
    }
    else
    {
        mock_ret = delegate_real_com_util_file_read(file, buf, len, read_out, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s len=%zu", __func__, len);
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
