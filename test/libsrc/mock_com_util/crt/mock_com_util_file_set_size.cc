#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_file_set_size(com_util_file *file, size_t size, com_util_error *detail_out)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_file_set_size)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_file_set_size"));

    return real_fn(file, size, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_file_set_size, com_util_file *file, size_t size, com_util_error *detail_out)
{
    if (_mock_com_util != nullptr)
    {
        return _mock_com_util->com_util_file_set_size(file, size, detail_out);
    }

    return delegate_real_com_util_file_set_size(file, size, detail_out);
}
