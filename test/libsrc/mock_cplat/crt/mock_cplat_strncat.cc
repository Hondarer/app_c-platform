#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_strncat(char *dest, size_t dest_size, const char *src, size_t count)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_strncat)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_strncat"));

    return real_fn(dest, dest_size, src, count);
}

MOCK_WEAK_IMPL(int, cplat_strncat, char *dest, size_t dest_size, const char *src, size_t count)
{
    int mock_ret = -1;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_strncat(dest, dest_size, src, count);
    }
    else
    {
        mock_ret = delegate_real_cplat_strncat(dest, dest_size, src, count);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu, %s, %zu", __func__, (void *)dest, dest_size, src, count);
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
