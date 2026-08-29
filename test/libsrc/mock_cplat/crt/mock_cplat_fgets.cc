#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_fgets(char *dest, size_t dest_size, FILE *stream, cplat_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_fgets)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_fgets"));

    return real_fn(dest, dest_size, stream, detail_out);
}

MOCK_WEAK_IMPL(int, cplat_fgets, char *dest, size_t dest_size, FILE *stream, cplat_error *detail_out)
{
    int mock_ret = -1;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_fgets(dest, dest_size, stream, detail_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_fgets(dest, dest_size, stream, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu, 0x%p", __func__, (void *)dest, dest_size, (void *)stream);
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
