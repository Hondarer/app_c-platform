#include <testfw.h>
#include <mock_com_util.h>

size_t delegate_real_com_util_fread(void *buffer, size_t size, size_t count, FILE *stream, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_fread)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_fread"));

    return real_fn(buffer, size, count, stream, detail_out);
}

MOCK_WEAK_IMPL(size_t, com_util_fread, void *buffer, size_t size, size_t count, FILE *stream,
               com_util_error *detail_out)
{
    if (_mock_com_util != nullptr)
    {
        return _mock_com_util->com_util_fread(buffer, size, count, stream, detail_out);
    }

    return delegate_real_com_util_fread(buffer, size, count, stream, detail_out);
}
