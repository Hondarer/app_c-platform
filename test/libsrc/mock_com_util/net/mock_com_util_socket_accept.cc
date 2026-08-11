#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_socket_accept(com_util_socket sock, com_util_ipv4_endpoint *peer_out, com_util_socket *sock_out, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_socket_accept)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_socket_accept"));

    return real_fn(sock, peer_out, sock_out, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_socket_accept, com_util_socket sock, com_util_ipv4_endpoint *peer_out, com_util_socket *sock_out, com_util_error *detail_out)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_socket_accept(sock, peer_out, sock_out, detail_out);
    }
    else
    {
        rtc = delegate_real_com_util_socket_accept(sock, peer_out, sock_out, detail_out);
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
