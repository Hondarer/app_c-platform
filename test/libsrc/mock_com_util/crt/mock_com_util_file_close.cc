#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_file_close(com_util_file *file, com_util_error *detail_out)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_file_close)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_file_close"));

    return real_fn(file, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_file_close, com_util_file *file, com_util_error *detail_out)
{
    int result;

    if (_mock_com_util != nullptr)
    {
        result = _mock_com_util->com_util_file_close(file, detail_out);
    }
    else
    {
        result = delegate_real_com_util_file_close(file, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p\n", __func__, (void *)file);
    }

    return result;
}
