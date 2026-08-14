/**
 *******************************************************************************
 *  @file           socket_linux.c
 *  @brief          IPv4 ソケット API の Linux 実装を提供します。
 *
 *  Doxygen コメントは、ヘッダーに記載
 *
 *******************************************************************************
 */

#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)

    #include <errno.h>
    #include <fcntl.h>
    #include <netinet/in.h>
    #include <poll.h>
    #include <string.h>
    #include <sys/socket.h>
    #include <unistd.h>

    #include <com_util/base/error_internal.h>
    #include <com_util/base/result.h>
    #include <com_util/clock/clock.h>
    #include <com_util/crt/unistd.h>
    #include <com_util/net/socket.h>
    #include <com_util/sync/sync.h>

/**
 *  @brief          共通の端点表現を sockaddr_in へ変換します。
 *  @param[in]      endpoint 変換元の端点。
 *  @param[out]     native   変換後の sockaddr_in の格納先。
 */
static void endpoint_to_native(const com_util_ipv4_endpoint *endpoint, struct sockaddr_in *native)
{
    memset(native, 0, sizeof(*native));
    native->sin_family = AF_INET;
    memcpy(&native->sin_addr.s_addr, &endpoint->address, sizeof(native->sin_addr.s_addr));
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
    memcpy(&endpoint->address, &native->sin_addr.s_addr, sizeof(endpoint->address));
    memcpy(&endpoint->port, &native->sin_port, sizeof(endpoint->port));
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

    if (setsockopt((int)sock, level, optname, &value, (socklen_t)sizeof(value)) != 0)
    {
        return com_util_error_report_socket_errno(detail_out, errno);
    }

    return com_util_error_report_success(detail_out);
}

/**
 *  @brief          シグナルによる中断を吸収して poll を実行します。
 *  @param[in,out]  fds        poll へ渡す記述子の配列。
 *  @param[in]      count      @p fds の要素数。
 *  @param[in]      timeout_ms タイムアウト時間 (ミリ秒)。負値は無期限を表します。
 *  @return         poll の戻り値を返します。失敗した場合は負値を返し、errno を保持します。
 *
 *  中断された場合は、単調時刻から残り時間を再計算して待機を継続します。
 *  要求した時間より早く復帰しないため、シグナルの配信によって呼び出し側の
 *  タイムアウト判定が変化しません。
 */
static int poll_with_deadline(struct pollfd *fds, nfds_t count, int timeout_ms)
{
    uint64_t deadline = 0U;
    int remaining_ms = timeout_ms;

    if (timeout_ms > 0)
    {
        deadline = com_util_get_monotonic_ms() + (uint64_t)timeout_ms;
    }

    for (;;)
    {
        const int poll_result = poll(fds, count, remaining_ms);

        if (poll_result >= 0)
        {
            return poll_result;
        }
        if (errno != EINTR)
        {
            return poll_result;
        }

        if (timeout_ms > 0)
        {
            const uint64_t now = com_util_get_monotonic_ms();

            if (now >= deadline)
            {
                /* 残り時間が尽きた。タイムアウトと同じく条件不成立を返す。 */
                return 0;
            }
            remaining_ms = (int)(deadline - now);
        }
    }
}

/**
 *  @brief          単一のソケットに対して poll による待機を行います。
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
    struct pollfd poll_fd;
    int poll_result;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (ready_out == NULL))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    *ready_out = 0;

    poll_fd.fd = (int)sock;
    poll_fd.events = events;
    poll_fd.revents = 0;

    poll_result = poll_with_deadline(&poll_fd, (nfds_t)1, timeout_ms);
    if (poll_result < 0)
    {
        return com_util_error_report_socket_errno(detail_out, errno);
    }

    if ((poll_result > 0) && ((poll_fd.revents & events) != 0))
    {
        *ready_out = 1;
    }

    return com_util_error_report_success(detail_out);
}

/**
 *  @brief          シグナルによる中断を吸収して accept を実行します。
 *  @param[in]      fd       待ち受けソケットの記述子。
 *  @param[out]     addr     接続元アドレスの格納先。
 *  @param[in,out]  addr_len @p addr のバイト数。復帰時に実際の長さが格納されます。
 *  @return         accept の戻り値を返します。失敗した場合は負値を返し、errno を保持します。
 */
static int retry_accept(int fd, struct sockaddr *addr, socklen_t *addr_len)
{
    int accepted;

    do
    {
        accepted = accept(fd, addr, addr_len);
    } while ((accepted < 0) && (errno == EINTR));

    return accepted;
}

/**
 *  @brief          SIGPIPE とシグナルによる中断を吸収して send を実行します。
 *  @param[in]      fd  対象のソケットの記述子。
 *  @param[in]      buf 送信するデータ。
 *  @param[in]      len @p buf のバイト数。
 *  @return         send の戻り値を返します。失敗した場合は負値を返し、errno を保持します。
 */
static ssize_t retry_send(int fd, const void *buf, size_t len)
{
    ssize_t transferred;

    do
    {
        /* MSG_NOSIGNAL はプロセス全体の設定を変更せず、この送信による SIGPIPE だけを抑制する。
         * EPIPE は従来どおり返される。
         * see: https://man7.org/linux/man-pages/man2/send.2.html */
        transferred = send(fd, buf, len, MSG_NOSIGNAL);
    } while ((transferred < 0) && (errno == EINTR));

    return transferred;
}

/**
 *  @brief          シグナルによる中断を吸収して recv を実行します。
 *  @param[in]      fd  対象のソケットの記述子。
 *  @param[out]     buf 受信したデータの格納先。
 *  @param[in]      len @p buf のバイト数。
 *  @return         recv の戻り値を返します。失敗した場合は負値を返し、errno を保持します。
 */
static ssize_t retry_recv(int fd, void *buf, size_t len)
{
    ssize_t transferred;

    do
    {
        transferred = recv(fd, buf, len, 0);
    } while ((transferred < 0) && (errno == EINTR));

    return transferred;
}

/**
 *  @brief          シグナルによる中断を吸収して sendto を実行します。
 *  @param[in]      fd       対象のソケットの記述子。
 *  @param[in]      buf      送信するデータ。
 *  @param[in]      len      @p buf のバイト数。
 *  @param[in]      addr     送信先アドレス。
 *  @param[in]      addr_len @p addr のバイト数。
 *  @return         sendto の戻り値を返します。失敗した場合は負値を返し、errno を保持します。
 */
static ssize_t retry_sendto(int fd, const void *buf, size_t len, const struct sockaddr *addr, socklen_t addr_len)
{
    ssize_t transferred;

    do
    {
        transferred = sendto(fd, buf, len, 0, addr, addr_len);
    } while ((transferred < 0) && (errno == EINTR));

    return transferred;
}

/**
 *  @brief          シグナルによる中断を吸収して recvfrom を実行します。
 *  @param[in]      fd       対象のソケットの記述子。
 *  @param[out]     buf      受信したデータの格納先。
 *  @param[in]      len      @p buf のバイト数。
 *  @param[out]     addr     送信元アドレスの格納先。
 *  @param[in,out]  addr_len @p addr のバイト数。復帰時に実際の長さが格納されます。
 *  @return         recvfrom の戻り値を返します。失敗した場合は負値を返し、errno を保持します。
 */
static ssize_t retry_recvfrom(int fd, void *buf, size_t len, struct sockaddr *addr, socklen_t *addr_len)
{
    ssize_t transferred;

    do
    {
        transferred = recvfrom(fd, buf, len, 0, addr, addr_len);
    } while ((transferred < 0) && (errno == EINTR));

    return transferred;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_open(const com_util_socket_kind kind, com_util_socket *sock_out, com_util_error *detail_out)
{
    int native_type;
    int native_socket;

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

    native_socket = socket(AF_INET, native_type, 0);
    if (native_socket < 0)
    {
        return com_util_error_report_socket_errno(detail_out, errno);
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

    /* com_util_close() は TLS の直前エラーを更新するため、解放経路で呼び出しても
       呼び出し前の診断情報が失われないように保存と復元を行う。 */
    com_util_error_get_last(&saved);
    /* EINTR で復帰した場合も再試行しない。Linux では中断された時点で記述子が
       解放済みであり、呼び直すと別スレッドが再利用した記述子を閉じうる。
       see: https://man7.org/linux/man-pages/man2/close.2.html */
    (void)com_util_close((int)sock, NULL);
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
    (void)shutdown((int)sock, SHUT_RDWR);
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

    if (bind((int)sock, (const struct sockaddr *)&native, (socklen_t)sizeof(native)) != 0)
    {
        return com_util_error_report_socket_errno(detail_out, errno);
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

    if (listen((int)sock, native_backlog) != 0)
    {
        return com_util_error_report_socket_errno(detail_out, errno);
    }

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_accept(const com_util_socket sock, com_util_ipv4_endpoint *peer_out, com_util_socket *sock_out,
                           com_util_error *detail_out)
{
    struct sockaddr_in native;
    socklen_t native_len = (socklen_t)sizeof(native);
    int accepted;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (sock_out == NULL))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    *sock_out = COM_UTIL_INVALID_SOCKET;
    memset(&native, 0, sizeof(native));

    accepted = retry_accept((int)sock, (struct sockaddr *)&native, &native_len);
    if (accepted < 0)
    {
        return com_util_error_report_socket_errno(detail_out, errno);
    }

    if (peer_out != NULL)
    {
        endpoint_from_native(&native, peer_out);
    }

    *sock_out = (com_util_socket)accepted;

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

/**
 *  @brief          中断された接続確立の完了を待機します。
 *  @param[in]      sock       対象のソケット。
 *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
 *  @return         共通結果コードを返します。
 */
static int wait_connect_completion(com_util_socket sock, com_util_error *detail_out)
{
    int ready = 0;
    const int wait_result = com_util_socket_wait_writable(sock, COM_UTIL_SOCKET_WAIT_FOREVER, &ready, detail_out);

    if (wait_result != COM_UTIL_OK)
    {
        return wait_result;
    }
    if (ready == 0)
    {
        return com_util_error_report_errno_as(detail_out, ETIMEDOUT, COM_UTIL_ERR_TIMEOUT);
    }

    return com_util_socket_get_pending_error(sock, detail_out);
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

    if (connect((int)sock, (const struct sockaddr *)&native, (socklen_t)sizeof(native)) != 0)
    {
        if (errno == EINPROGRESS)
        {
            return com_util_error_report_socket_errno_as(detail_out, errno, COM_UTIL_ERR_IN_PROGRESS);
        }
        if (errno != EINTR)
        {
            return com_util_error_report_socket_errno(detail_out, errno);
        }

        /* シグナルで中断された接続確立は非同期に継続する。connect を呼び直すと
           EALREADY または EISCONN になるため、書き込み可能になるまで待機し、
           SO_ERROR で結果を確認する。
           see: https://man7.org/linux/man-pages/man2/connect.2.html */
        return wait_connect_completion(sock, detail_out);
    }

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_get_pending_error(const com_util_socket sock, com_util_error *detail_out)
{
    int pending = 0;
    socklen_t pending_len = (socklen_t)sizeof(pending);

    if (sock == COM_UTIL_INVALID_SOCKET)
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    if (getsockopt((int)sock, SOL_SOCKET, SO_ERROR, &pending, &pending_len) != 0)
    {
        return com_util_error_report_socket_errno(detail_out, errno);
    }

    if (pending != 0)
    {
        return com_util_error_report_socket_errno(detail_out, pending);
    }

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_set_nonblocking(const com_util_socket sock, const int enable, com_util_error *detail_out)
{
    int flags;
    int updated;

    if (sock == COM_UTIL_INVALID_SOCKET)
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    flags = fcntl((int)sock, F_GETFL, 0);
    if (flags < 0)
    {
        return com_util_error_report_socket_errno(detail_out, errno);
    }

    if (enable != 0)
    {
        updated = flags | O_NONBLOCK;
    }
    else
    {
        updated = flags & ~O_NONBLOCK;
    }

    if (fcntl((int)sock, F_SETFL, updated) < 0)
    {
        return com_util_error_report_socket_errno(detail_out, errno);
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
    memcpy(&value.s_addr, &interface_address, sizeof(value.s_addr));

    if (setsockopt((int)sock, IPPROTO_IP, IP_MULTICAST_IF, &value, (socklen_t)sizeof(value)) != 0)
    {
        return com_util_error_report_socket_errno(detail_out, errno);
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
    memcpy(&request.imr_multiaddr.s_addr, &group_address, sizeof(request.imr_multiaddr.s_addr));
    memcpy(&request.imr_interface.s_addr, &interface_address, sizeof(request.imr_interface.s_addr));

    if (setsockopt((int)sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &request, (socklen_t)sizeof(request)) != 0)
    {
        return com_util_error_report_socket_errno(detail_out, errno);
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
    memcpy(&request.imr_multiaddr.s_addr, &group_address, sizeof(request.imr_multiaddr.s_addr));
    memcpy(&request.imr_interface.s_addr, &interface_address, sizeof(request.imr_interface.s_addr));

    if (setsockopt((int)sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &request, (socklen_t)sizeof(request)) != 0)
    {
        return com_util_error_report_socket_errno(detail_out, errno);
    }

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_send(const com_util_socket sock, const void *buf, const size_t len, size_t *sent_out,
                         com_util_error *detail_out)
{
    ssize_t transferred;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (buf == NULL) || (sent_out == NULL) ||
        (len > COM_UTIL_SOCKET_MAX_TRANSFER))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    *sent_out = 0U;

    transferred = retry_send((int)sock, buf, len);
    if (transferred < 0)
    {
        return com_util_error_report_socket_errno(detail_out, errno);
    }

    *sent_out = (size_t)transferred;

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_recv(const com_util_socket sock, void *buf, const size_t len, size_t *received_out,
                         com_util_error *detail_out)
{
    ssize_t transferred;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (buf == NULL) || (received_out == NULL) ||
        (len > COM_UTIL_SOCKET_MAX_TRANSFER))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    *received_out = 0U;

    transferred = retry_recv((int)sock, buf, len);
    if (transferred < 0)
    {
        return com_util_error_report_socket_errno(detail_out, errno);
    }

    *received_out = (size_t)transferred;

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_sendto(const com_util_socket sock, const void *buf, const size_t len,
                           const com_util_ipv4_endpoint *endpoint, size_t *sent_out, com_util_error *detail_out)
{
    struct sockaddr_in native;
    ssize_t transferred;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (buf == NULL) || (endpoint == NULL) || (sent_out == NULL) ||
        (len > COM_UTIL_SOCKET_MAX_TRANSFER))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    *sent_out = 0U;
    endpoint_to_native(endpoint, &native);

    transferred = retry_sendto((int)sock, buf, len, (const struct sockaddr *)&native, (socklen_t)sizeof(native));
    if (transferred < 0)
    {
        return com_util_error_report_socket_errno(detail_out, errno);
    }

    *sent_out = (size_t)transferred;

    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_recvfrom(const com_util_socket sock, void *buf, const size_t len,
                             com_util_ipv4_endpoint *peer_out, size_t *received_out, com_util_error *detail_out)
{
    struct sockaddr_in native;
    socklen_t native_len = (socklen_t)sizeof(native);
    ssize_t transferred;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (buf == NULL) || (received_out == NULL) ||
        (len > COM_UTIL_SOCKET_MAX_TRANSFER))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    *received_out = 0U;
    memset(&native, 0, sizeof(native));

    transferred = retry_recvfrom((int)sock, buf, len, (struct sockaddr *)&native, &native_len);
    if (transferred < 0)
    {
        return com_util_error_report_socket_errno(detail_out, errno);
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
    const uint8_t *cursor = (const uint8_t *)buf;
    size_t sent = 0U;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (buf == NULL) || (len > COM_UTIL_SOCKET_MAX_TRANSFER))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    while (sent < len)
    {
        const ssize_t transferred = retry_send((int)sock, cursor + sent, len - sent);

        if (transferred < 0)
        {
            return com_util_error_report_socket_errno(detail_out, errno);
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
    uint8_t *cursor = (uint8_t *)buf;
    size_t received = 0U;

    if ((sock == COM_UTIL_INVALID_SOCKET) || (buf == NULL) || (len > COM_UTIL_SOCKET_MAX_TRANSFER))
    {
        return com_util_error_report_errno_as(detail_out, EINVAL, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    while (received < len)
    {
        const ssize_t transferred = retry_recv((int)sock, cursor + received, len - received);

        if (transferred < 0)
        {
            return com_util_error_report_socket_errno(detail_out, errno);
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
    return wait_single(sock, (short)POLLIN, timeout_ms, ready_out, detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_wait_writable(const com_util_socket sock, const int timeout_ms, int *ready_out,
                                  com_util_error *detail_out)
{
    return wait_single(sock, (short)POLLOUT, timeout_ms, ready_out, detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_socket_wait_readable_multi(const com_util_socket *socks, const size_t count, const int timeout_ms,
                                        unsigned char *ready_out, com_util_error *detail_out)
{
    struct pollfd poll_fds[COM_UTIL_SOCKET_WAIT_MAX];
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
            poll_fds[valid_count].fd = (int)socks[index];
            poll_fds[valid_count].events = (short)POLLIN;
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

    poll_result = poll_with_deadline(poll_fds, (nfds_t)valid_count, timeout_ms);
    if (poll_result < 0)
    {
        return com_util_error_report_socket_errno(detail_out, errno);
    }

    valid_count = 0U;
    for (index = 0U; index < count; ++index)
    {
        if (socks[index] != COM_UTIL_INVALID_SOCKET)
        {
            const short revents = poll_fds[valid_count].revents;

            if ((revents & (POLLIN | POLLERR | POLLHUP | POLLNVAL)) != 0)
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

    /* Linux は受信方向の停止で待機が解除されるため、ハンドルを保持したままにする。 */
    if (shutdown((int)*sock_inout, SHUT_RD) != 0)
    {
        return com_util_error_report_socket_errno(detail_out, errno);
    }

    return com_util_error_report_success(detail_out);
}

#elif defined(PLATFORM_WINDOWS) && defined(COMPILER_MSVC)
    #pragma warning(disable : 4206)
#endif /* PLATFORM_ */
