#include <testfw.h>
#include <mock_cplat.h>

int delegate_real_cplat_console_attach_parent(int *argc, char **argv, int *attached_out)
{
    static auto real_fn = reinterpret_cast<decltype(&cplat_console_attach_parent)>(
        resolveSharedSymbolOrExit(kLibCplatName, "cplat_console_attach_parent"));

    return real_fn(argc, argv, attached_out);
}

MOCK_WEAK_IMPL(int, cplat_console_attach_parent, int *argc, char **argv, int *attached_out)
{
    int mock_ret = CPLAT_ERR_UNKNOWN;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->cplat_console_attach_parent(argc, argv, attached_out);
    }
    else
    {
        mock_ret = delegate_real_cplat_console_attach_parent(argc, argv, attached_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
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
