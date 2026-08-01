#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_fclose(FILE *stream, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_fclose)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_fclose"));

    return real_fn(stream, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_fclose, FILE *stream, com_util_error *detail_out)
{
    if (_mock_com_util != nullptr)
    {
        return _mock_com_util->com_util_fclose(stream, detail_out);
    }

    return delegate_real_com_util_fclose(stream, detail_out);
}
