#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_socket_join_multicast_group(com_util_socket sock, uint32_t group_address, uint32_t interface_address, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_socket_join_multicast_group)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_socket_join_multicast_group"));

    return real_fn(sock, group_address, interface_address, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_socket_join_multicast_group, com_util_socket sock, uint32_t group_address, uint32_t interface_address, com_util_error *detail_out)
{
    int mock_ret = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_socket_join_multicast_group(sock, group_address, interface_address, detail_out);
    }
    else
    {
        mock_ret = delegate_real_com_util_socket_join_multicast_group(sock, group_address, interface_address, detail_out);
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
