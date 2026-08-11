#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_socket_wait_readable_multi(const com_util_socket *socks, size_t count, int timeout_ms, unsigned char *ready_out, com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_socket_wait_readable_multi)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_socket_wait_readable_multi"));

    return real_fn(socks, count, timeout_ms, ready_out, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_socket_wait_readable_multi, const com_util_socket *socks, size_t count, int timeout_ms, unsigned char *ready_out, com_util_error *detail_out)
{
    int rtc = COM_UTIL_ERR_UNKNOWN;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_socket_wait_readable_multi(socks, count, timeout_ms, ready_out, detail_out);
    }
    else
    {
        rtc = delegate_real_com_util_socket_wait_readable_multi(socks, count, timeout_ms, ready_out, detail_out);
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
