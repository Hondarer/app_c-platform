#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_get_temp_dir(char *path_out, size_t path_size, cplat_error *detail_out)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_get_temp_dir)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_get_temp_dir"));

    return real_fn(path_out, path_size, detail_out);
}

MOCK_WEAK_IMPL(int, cplat_get_temp_dir, char *path_out, size_t path_size, cplat_error *detail_out)
{
    int mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_get_temp_dir(path_out, path_size, detail_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_get_temp_dir(path_out, path_size, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }

    return mock_ret;
}
