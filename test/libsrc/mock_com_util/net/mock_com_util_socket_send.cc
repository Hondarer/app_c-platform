#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_socket_send(com_util_socket sock, const void *buf, size_t len, size_t *sent_out, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_socket_send)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_socket_send"));

    return real_fn(sock, buf, len, sent_out, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_socket_send, com_util_socket sock, const void *buf, size_t len, size_t *sent_out, com_util_error *detail_out)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_socket_send(sock, buf, len, sent_out, detail_out);
    }
    else
    {
        rtc = delegate_real_com_util_socket_send(sock, buf, len, sent_out, detail_out);
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
