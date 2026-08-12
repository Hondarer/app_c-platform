/**
 *******************************************************************************
 *  @file           socket_windows.c
 *  @brief          IPv4 ソケット API の Windows 実装を提供します。
 *
 *  Doxygen コメントは、ヘッダーに記載
 *
 *******************************************************************************
 */

#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)

    #include <com_util/base/windows_sdk.h>
    #include <errno.h>
    #include <string.h>

    #include <com_util/base/error_internal.h>
    #include <com_util/base/result.h>
    #include <com_util/net/socket.h>
    #include <com_util/net/socket_internal.h>
    #include <com_util/sync/sync.h>

/** Winsock の初期化を 1 回に限定するためのフラグ。 */
static com_util_once_flag s_startup_once = {0};

/** WSAStartup() の結果。0 は成功、それ以外は Winsock エラー コード。 */
static unsigned long s_startup_error = 0UL;

/** WSAStartup() が成功したことを示す値。 */
static int s_startup_done = 0;

/**
 *  @brief          Winsock を 1 回だけ初期化します。
 */
static void startup_once(void)
{
    WSADATA wsa_data;
    int startup_result;

    memset(&wsa_data, 0, sizeof(wsa_data));

    startup_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (startup_result != 0)
    {
        /* WSAStartup() は失敗理由を戻り値で返す。この時点では WSAGetLastError() を
           呼び出せないため、戻り値をそのまま記録する。
           see: https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-wsastartup */
        s_startup_error = (unsigned long)startup_result;
        return;
    }

    s_startup_done = 1;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_internal_socket_startup(com_util_error *detail_out)
{
    com_util_call_once(&s_startup_once, startup_once);

    if (s_startup_done == 0)
    {
        return com_util_error_report_winsock_error(detail_out, s_startup_error);
    }

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_internal_socket_cleanup(void)
{
    if (s_startup_done == 0)
    {
        return;
    }

    (void)WSACleanup();
    s_startup_done = 0;
}

/**
 *  @brief          共通の端点表現を sockaddr_in へ変換します。
 *  @param[in]      endpoint 変換元の端点。
 *  @param[out]     native   変換後の sockaddr_in の格納先。
 */
static void endpoint_to_native(const com_util_ipv4_endpoint *endpoint, struct sockaddr_in *native)
{
    memset(native, 0, sizeof(*native));
    native->sin_family = AF_INET;
    memcpy(&native->sin_addr.S_un.S_addr, &endpoint->address, sizeof(native->sin_addr.S_un.S_addr));
    memcpy(&native->sin_port, &endpoint->port, sizeof(native->sin_port));
}

/**
 *  @brief          sockaddr_in を共通の端点表現へ変換します。
 *  @param[in]      native   変換元の sockaddr_in。
 *  @param[out]     endpoint 変換後の端点の格納先。
 */
static void endpoint_from_native(const struct sockaddr_in *native, com_util_ipv4_endpoint *endpoint)
{
    memset(endpoint, 0, sizeof(*endpoint));
    memcpy(&endpoint->address, &native->sin_addr.S_un.S_addr, sizeof(endpoint->address));
    memcpy(&endpoint->port, &native->sin_port, sizeof(endpoint->port));
}

/**
 *  @brief          直前の Winsock エラーを記録します。
 *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
 *  @return         共通結果コードを返します。
 */
static int report_last_winsock_error(com_util_error *detail_out)
{
    return com_util_error_report_winsock_error(detail_out, (unsigned long)WSAGetLastError());
}

/**
 *  @brief          connect の非同期継続状態を共通結果コードへ変換します。
 *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
 *  @param[in]      error_code WSAGetLastError() が返した値。
 *  @return         共通結果コードを返します。
 *
 *  WSAEWOULDBLOCK は一般のソケット操作では WOULD_BLOCK ですが、
 *  非ブロッキング connect では接続処理が継続中であることを表します。
 *  生の Winsock エラーは detail_out に保持し、戻り値だけを IN_PROGRESS へ正規化します。
 *  see: https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-connect
 */
static int report_connect_winsock_error(com_util_error *detail_out, const unsigned long error_code)
{
    if ((error_code == (unsigned long)WSAEWOULDBLOCK) ||
        (error_code == (unsigned long)WSAEINPROGRESS) ||
        (error_code == (unsigned long)WSAEALREADY))
    {
        return com_util_error_report_winsock_error_as(detail_out, error_code, COM_UTIL_ERR_IN_PROGRESS);
    }

    return com_util_error_report_winsock_error(detail_out, error_code);
}

/**
 *  @brief          ソケット オプションへ整数値を設定します。
 *  @param[in]      sock       対象のソケット。
 *  @param[in]      level      オプションの階層。
 *  @param[in]      optname    オプション名。
 *  @param[in]      value      設定する値。
 *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
 *  @return         共通結果コードを返します。
 */
static int set_int_option(com_util_socket sock, int level, int optname, int value, com_util_error *detail_out)
{
    if (sock == COM_UTIL_INVALID_SOCKET)
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    if (setsockopt((SOCKET)sock, level, optname, (const char *)&value, (int)sizeof(value)) == SOCKET_ERROR)
    {
        return report_last_winsock_error(detail_out);
    }

    return com_util_error_report_success(detail_out);
}

/**
 *  @brief          単一のソケットに対して WSAPoll による待機を行います。
 *  @param[in]      sock       対象のソケット。
 *  @param[in]      events     待機するイベント。
 *  @param[in]      timeout_ms タイムアウト時間 (ミリ秒)。
 *  @param[out]     ready_out  条件が成立した場合に 1、タイムアウトした場合に 0 を格納します。
 *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
 *  @return         共通結果コードを返します。
 */
static int wait_single(com_util_socket sock, short events, int timeout_ms, int *ready_out,
                       com_util_error *detail_out)
{
    WSAPOLLFD poll_fd;
    int poll_result;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (ready_out == NULL))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    *ready_out = 0;

    poll_fd.fd = (SOCKET)sock;
    poll_fd.events = events;
    poll_fd.revents = 0;

    poll_result = WSAPoll(&poll_fd, (ULONG)1, timeout_ms);
    if (poll_result == SOCKET_ERROR)
    {
        return report_last_winsock_error(detail_out);
    }

    if ((poll_result > 0) && ((poll_fd.revents & events) != 0))
    {
        *ready_out = 1;
    }

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_open(const com_util_socket_kind kind, com_util_socket *sock_out, com_util_error *detail_out)
{
    int native_type;
    int startup_result;
    SOCKET native_socket;

    if (sock_out == NULL)
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    *sock_out = COM_UTIL_INVALID_SOCKET;

    if (kind == COM_UTIL_SOCKET_TCP)
    {
        native_type = SOCK_STREAM;
    }
    else if (kind == COM_UTIL_SOCKET_UDP)
    {
        native_type = SOCK_DGRAM;
    }
    else
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    startup_result = com_util_internal_socket_startup(detail_out);
    if (startup_result != COM_UTIL_OK)
    {
        return startup_result;
    }

    native_socket = socket(AF_INET, native_type, 0);
    if (native_socket == INVALID_SOCKET)
    {
        return report_last_winsock_error(detail_out);
    }

    *sock_out = (com_util_socket)native_socket;

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_socket_close(const com_util_socket sock)
{
    com_util_error saved;

    if (sock == COM_UTIL_INVALID_SOCKET)
    {
        return;
    }

    /* 解放経路で呼び出しても呼び出し前の診断情報が失われないように保存と復元を行う。 */
    com_util_error_get_last(&saved);
    (void)closesocket((SOCKET)sock);
    com_util_error_set_last(&saved);
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_socket_shutdown(const com_util_socket sock)
{
    if (sock == COM_UTIL_INVALID_SOCKET)
    {
        return;
    }

    /* 失敗しても呼び出し側に取るべき手段がないため、結果は参照しない。 */
    (void)shutdown((SOCKET)sock, SD_BOTH);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_bind(const com_util_socket sock, const com_util_ipv4_endpoint *endpoint,
                         com_util_error *detail_out)
{
    struct sockaddr_in native;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (endpoint == NULL))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    endpoint_to_native(endpoint, &native);

    if (bind((SOCKET)sock, (const struct sockaddr *)&native, (int)sizeof(native)) == SOCKET_ERROR)
    {
        return report_last_winsock_error(detail_out);
    }

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_listen(const com_util_socket sock, const int backlog, com_util_error *detail_out)
{
    int native_backlog = backlog;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (backlog < 0))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    if (backlog == COM_UTIL_SOCKET_BACKLOG_DEFAULT)
    {
        native_backlog = SOMAXCONN;
    }

    if (listen((SOCKET)sock, native_backlog) == SOCKET_ERROR)
    {
        return report_last_winsock_error(detail_out);
    }

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_accept(const com_util_socket sock, com_util_ipv4_endpoint *peer_out, com_util_socket *sock_out,
                           com_util_error *detail_out)
{
    struct sockaddr_in native;
    int native_len = (int)sizeof(native);
    SOCKET accepted;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (sock_out == NULL))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    *sock_out = COM_UTIL_INVALID_SOCKET;
    memset(&native, 0, sizeof(native));

    accepted = accept((SOCKET)sock, (struct sockaddr *)&native, &native_len);
    if (accepted == INVALID_SOCKET)
    {
        return report_last_winsock_error(detail_out);
    }

    if (peer_out != NULL)
    {
        endpoint_from_native(&native, peer_out);
    }

    *sock_out = (com_util_socket)accepted;

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_connect(const com_util_socket sock, const com_util_ipv4_endpoint *endpoint,
                            com_util_error *detail_out)
{
    struct sockaddr_in native;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (endpoint == NULL))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    endpoint_to_native(endpoint, &native);

    if (connect((SOCKET)sock, (const struct sockaddr *)&native, (int)sizeof(native)) == SOCKET_ERROR)
    {
        return report_connect_winsock_error(detail_out, (unsigned long)WSAGetLastError());
    }

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_get_pending_error(const com_util_socket sock, com_util_error *detail_out)
{
    int pending = 0;
    int pending_len = (int)sizeof(pending);

    if (sock == COM_UTIL_INVALID_SOCKET)
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    if (getsockopt((SOCKET)sock, SOL_SOCKET, SO_ERROR, (char *)&pending, &pending_len) == SOCKET_ERROR)
    {
        return report_last_winsock_error(detail_out);
    }

    if (pending != 0)
    {
        return com_util_error_report_winsock_error(detail_out, (unsigned long)pending);
    }

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_set_nonblocking(const com_util_socket sock, const int enable, com_util_error *detail_out)
{
    u_long mode = 0UL;

    if (sock == COM_UTIL_INVALID_SOCKET)
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    if (enable != 0)
    {
        mode = 1UL;
    }

    /* Windows には fcntl(F_SETFL) 相当がないため、FIONBIO で切り替える。
       see: https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-ioctlsocket */
    if (ioctlsocket((SOCKET)sock, (long)FIONBIO, &mode) == SOCKET_ERROR)
    {
        return report_last_winsock_error(detail_out);
    }

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_set_reuse_address(const com_util_socket sock, const int enable, com_util_error *detail_out)
{
    int value = 0;

    if (enable != 0)
    {
        value = 1;
    }

    return set_int_option(sock, SOL_SOCKET, SO_REUSEADDR, value, detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_set_broadcast(const com_util_socket sock, const int enable, com_util_error *detail_out)
{
    int value = 0;

    if (enable != 0)
    {
        value = 1;
    }

    return set_int_option(sock, SOL_SOCKET, SO_BROADCAST, value, detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_set_multicast_interface(const com_util_socket sock, const uint32_t interface_address,
                                            com_util_error *detail_out)
{
    struct in_addr value;

    if (sock == COM_UTIL_INVALID_SOCKET)
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    memset(&value, 0, sizeof(value));
    memcpy(&value.S_un.S_addr, &interface_address, sizeof(value.S_un.S_addr));

    if (setsockopt((SOCKET)sock, IPPROTO_IP, IP_MULTICAST_IF, (const char *)&value, (int)sizeof(value)) ==
        SOCKET_ERROR)
    {
        return report_last_winsock_error(detail_out);
    }

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_join_multicast_group(const com_util_socket sock, const uint32_t group_address,
                                         const uint32_t interface_address, com_util_error *detail_out)
{
    struct ip_mreq request;

    if (sock == COM_UTIL_INVALID_SOCKET)
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    memset(&request, 0, sizeof(request));
    memcpy(&request.imr_multiaddr.S_un.S_addr, &group_address, sizeof(request.imr_multiaddr.S_un.S_addr));
    memcpy(&request.imr_interface.S_un.S_addr, &interface_address, sizeof(request.imr_interface.S_un.S_addr));

    if (setsockopt((SOCKET)sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&request, (int)sizeof(request)) ==
        SOCKET_ERROR)
    {
        return report_last_winsock_error(detail_out);
    }

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_leave_multicast_group(const com_util_socket sock, const uint32_t group_address,
                                          const uint32_t interface_address, com_util_error *detail_out)
{
    struct ip_mreq request;

    if (sock == COM_UTIL_INVALID_SOCKET)
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    memset(&request, 0, sizeof(request));
    memcpy(&request.imr_multiaddr.S_un.S_addr, &group_address, sizeof(request.imr_multiaddr.S_un.S_addr));
    memcpy(&request.imr_interface.S_un.S_addr, &interface_address, sizeof(request.imr_interface.S_un.S_addr));

    if (setsockopt((SOCKET)sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const char *)&request, (int)sizeof(request)) ==
        SOCKET_ERROR)
    {
        return report_last_winsock_error(detail_out);
    }

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_send(const com_util_socket sock, const void *buf, const size_t len, size_t *sent_out,
                         com_util_error *detail_out)
{
    int transferred;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (buf == NULL) || (sent_out == NULL) ||
        (len > COM_UTIL_SOCKET_MAX_TRANSFER))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    *sent_out = 0U;

    transferred = send((SOCKET)sock, (const char *)buf, (int)len, 0);
    if (transferred == SOCKET_ERROR)
    {
        return report_last_winsock_error(detail_out);
    }

    *sent_out = (size_t)transferred;

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_recv(const com_util_socket sock, void *buf, const size_t len, size_t *received_out,
                         com_util_error *detail_out)
{
    int transferred;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (buf == NULL) || (received_out == NULL) ||
        (len > COM_UTIL_SOCKET_MAX_TRANSFER))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    *received_out = 0U;

    transferred = recv((SOCKET)sock, (char *)buf, (int)len, 0);
    if (transferred == SOCKET_ERROR)
    {
        return report_last_winsock_error(detail_out);
    }

    *received_out = (size_t)transferred;

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_sendto(const com_util_socket sock, const void *buf, const size_t len,
                           const com_util_ipv4_endpoint *endpoint, size_t *sent_out, com_util_error *detail_out)
{
    struct sockaddr_in native;
    int transferred;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (buf == NULL) || (endpoint == NULL) || (sent_out == NULL) ||
        (len > COM_UTIL_SOCKET_MAX_TRANSFER))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    *sent_out = 0U;
    endpoint_to_native(endpoint, &native);

    transferred =
        sendto((SOCKET)sock, (const char *)buf, (int)len, 0, (const struct sockaddr *)&native, (int)sizeof(native));
    if (transferred == SOCKET_ERROR)
    {
        return report_last_winsock_error(detail_out);
    }

    *sent_out = (size_t)transferred;

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_recvfrom(const com_util_socket sock, void *buf, const size_t len,
                             com_util_ipv4_endpoint *peer_out, size_t *received_out, com_util_error *detail_out)
{
    struct sockaddr_in native;
    int native_len = (int)sizeof(native);
    int transferred;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (buf == NULL) || (received_out == NULL) ||
        (len > COM_UTIL_SOCKET_MAX_TRANSFER))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    *received_out = 0U;
    memset(&native, 0, sizeof(native));

    transferred = recvfrom((SOCKET)sock, (char *)buf, (int)len, 0, (struct sockaddr *)&native, &native_len);
    if (transferred == SOCKET_ERROR)
    {
        return report_last_winsock_error(detail_out);
    }

    if (peer_out != NULL)
    {
        endpoint_from_native(&native, peer_out);
    }

    *received_out = (size_t)transferred;

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_send_all(const com_util_socket sock, const void *buf, const size_t len,
                             com_util_error *detail_out)
{
    const char *cursor = (const char *)buf;
    size_t sent = 0U;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (buf == NULL) || (len > COM_UTIL_SOCKET_MAX_TRANSFER))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    while (sent < len)
    {
        int transferred = send((SOCKET)sock, cursor + sent, (int)(len - sent), 0);

        if (transferred == SOCKET_ERROR)
        {
            return report_last_winsock_error(detail_out);
        }
        if (transferred == 0)
        {
            return com_util_error_report_errno_as(detail_out, EIO, COM_UTIL_ERR_UNKNOWN);
        }

        sent += (size_t)transferred;
    }

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_recv_all(const com_util_socket sock, void *buf, const size_t len, com_util_error *detail_out)
{
    char *cursor = (char *)buf;
    size_t received = 0U;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (buf == NULL) || (len > COM_UTIL_SOCKET_MAX_TRANSFER))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    while (received < len)
    {
        int transferred = recv((SOCKET)sock, cursor + received, (int)(len - received), 0);

        if (transferred == SOCKET_ERROR)
        {
            return report_last_winsock_error(detail_out);
        }
        if (transferred == 0)
        {
            return com_util_error_report_errno_as(detail_out, 0, COM_UTIL_ERR_EOF);
        }

        received += (size_t)transferred;
    }

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_wait_readable(const com_util_socket sock, const int timeout_ms, int *ready_out,
                                  com_util_error *detail_out)
{
    return wait_single(sock, (short)POLLRDNORM, timeout_ms, ready_out, detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_wait_writable(const com_util_socket sock, const int timeout_ms, int *ready_out,
                                  com_util_error *detail_out)
{
    return wait_single(sock, (short)POLLWRNORM, timeout_ms, ready_out, detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_wait_readable_multi(const com_util_socket *socks, const size_t count, const int timeout_ms,
                                        unsigned char *ready_out, com_util_error *detail_out)
{
    WSAPOLLFD poll_fds[COM_UTIL_SOCKET_WAIT_MAX];
    size_t valid_count = 0U;
    size_t index;
    int poll_result;

    if ((socks == NULL) || (ready_out == NULL) || (count == 0U) || (count > COM_UTIL_SOCKET_WAIT_MAX))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    for (index = 0U; index < count; ++index)
    {
        ready_out[index] = 0U;
        if (socks[index] != COM_UTIL_INVALID_SOCKET)
        {
            poll_fds[valid_count].fd = (SOCKET)socks[index];
            poll_fds[valid_count].events = (short)POLLRDNORM;
            poll_fds[valid_count].revents = 0;
            ++valid_count;
        }
    }

    if (valid_count == 0U)
    {
        /* 有効なソケットがない場合も timeout_ms だけ待つ。即座に返すと呼び出し側の
           ポーリング ループが待機なしで回り続けるため。 */
        if (timeout_ms > 0)
        {
            com_util_sleep_ms(timeout_ms);
        }
        return com_util_error_report_success(detail_out);
    }

    poll_result = WSAPoll(poll_fds, (ULONG)valid_count, timeout_ms);
    if (poll_result == SOCKET_ERROR)
    {
        return report_last_winsock_error(detail_out);
    }

    valid_count = 0U;
    for (index = 0U; index < count; ++index)
    {
        if (socks[index] != COM_UTIL_INVALID_SOCKET)
        {
            const short revents = poll_fds[valid_count].revents;

            if ((revents & (POLLRDNORM | POLLERR | POLLHUP | POLLNVAL)) != 0)
            {
                ready_out[index] = 1U;
            }
            ++valid_count;
        }
    }

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_shutdown_receive(com_util_socket *sock_inout, com_util_error *detail_out)
{
    if ((sock_inout == NULL) || (*sock_inout == COM_UTIL_INVALID_SOCKET))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    /* Windows は受信方向の半クローズでは待機中の recv が解除されないため、
       ソケットを閉じて呼び出し側へ無効値を書き戻す。 */
    if (closesocket((SOCKET)*sock_inout) == SOCKET_ERROR)
    {
        return report_last_winsock_error(detail_out);
    }

    *sock_inout = COM_UTIL_INVALID_SOCKET;

    return com_util_error_report_success(detail_out);
}

#endif /* PLATFORM_WINDOWS */
