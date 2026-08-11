#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_socket_set_reuse_address(com_util_socket sock, int enable, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_socket_set_reuse_address)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_socket_set_reuse_address"));

    return real_fn(sock, enable, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_socket_set_reuse_address, com_util_socket sock, int enable, com_util_error *detail_out)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_socket_set_reuse_address(sock, enable, detail_out);
    }
    else
    {
        rtc = delegate_real_com_util_socket_set_reuse_address(sock, enable, detail_out);
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
