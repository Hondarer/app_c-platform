#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_socket_close(com_util_socket sock)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_socket_close)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_socket_close"));

    real_fn(sock);
}

MOCK_WEAK_IMPL(void, com_util_socket_close, com_util_socket sock)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_socket_close(sock);
    }
    else
    {
        delegate_real_com_util_socket_close(sock);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s\n", __func__);
    }
}
