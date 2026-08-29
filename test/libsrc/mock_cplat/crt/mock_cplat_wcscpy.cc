#include <wchar.h>
#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_wcscpy(wchar_t *dest, size_t dest_size, const wchar_t *src)
{
    static auto real_fn =
        reinterpret_cast<decltype(&cplat_wcscpy)>(resolveSharedSymbolOrExit(kLibCplatName, "cplat_wcscpy"));

    return real_fn(dest, dest_size, src);
}

MOCK_WEAK_IMPL(int, cplat_wcscpy, wchar_t *dest, size_t dest_size, const wchar_t *src)
{
    int mock_ret = -1;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_wcscpy(dest, dest_size, src);
    }
    else
    {
        mock_ret = delegate_real_cplat_wcscpy(dest, dest_size, src);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu, 0x%p", __func__, (void *)dest, dest_size, (const void *)src);
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
