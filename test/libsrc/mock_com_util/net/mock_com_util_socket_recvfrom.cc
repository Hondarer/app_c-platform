#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_socket_recvfrom(com_util_socket sock, void *buf, size_t len, com_util_ipv4_endpoint *peer_out, size_t *received_out, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_socket_recvfrom)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_socket_recvfrom"));

    return real_fn(sock, buf, len, peer_out, received_out, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_socket_recvfrom, com_util_socket sock, void *buf, size_t len, com_util_ipv4_endpoint *peer_out, size_t *received_out, com_util_error *detail_out)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_socket_recvfrom(sock, buf, len, peer_out, received_out, detail_out);
    }
    else
    {
        mock_ret = delegate_real_com_util_socket_recvfrom(sock, buf, len, peer_out, received_out, detail_out);
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
