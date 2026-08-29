#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_file_get_path_id(const char *path, cplat_file_id *id_out, cplat_error *detail_out)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_file_get_path_id)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_file_get_path_id"));

    return real_fn(path, id_out, detail_out);
}

MOCK_WEAK_IMPL(int, cplat_file_get_path_id, const char *path, cplat_file_id *id_out, cplat_error *detail_out)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_file_get_path_id(path, id_out, detail_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_file_get_path_id(path, id_out, detail_out);
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
            printf(" -> %d\n", mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}
