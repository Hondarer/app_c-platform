#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_console_attach_parent(int *argc, char **argv, int *attached_out)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_console_attach_parent)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_console_attach_parent"));

    return real_fn(argc, argv, attached_out);
}

MOCK_WEAK_IMPL(int, com_util_console_attach_parent, int *argc, char **argv, int *attached_out)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_console_attach_parent(argc, argv, attached_out);
    }
    else
    {
        rtc = delegate_real_com_util_console_attach_parent(argc, argv, attached_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s", __func__);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}
