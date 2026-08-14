#include <testfw.h>

#include <mock_com_util.h>

#include <com_util/base/platform.h>
#include <com_util/base/result.h>
#include <com_util/net/socket.h>

#if defined(PLATFORM_LINUX)
    #include <mock_fcntl.h>
    #include <mock_poll.h>
    #include <sys/mock_socket.h>

    #include <errno.h>
    #include <fcntl.h>
    #include <netinet/in.h>
    #include <poll.h>
    #include <sys/socket.h>
#elif defined(PLATFORM_WINDOWS)
    #include <com_util/net/socket_internal.h>
    #include <mock_winsock.h>

    #include <errno.h>
#endif /* PLATFORM_ */

#include <cstring>

using testing::_;
using testing::Assign;
using testing::DoAll;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

namespace
{

const com_util_socket kSocket = (com_util_socket)7;

const com_util_ipv4_endpoint kEndpoint = {COM_UTIL_IPV4_ADDR_LOOPBACK, com_util_hton16((uint16_t)12345U), 0U};

void expect_detail(const com_util_error &detail, const com_util_error_domain domain, const int result,
                   const unsigned long code)
{
    EXPECT_EQ(domain, detail.domain);
    EXPECT_EQ(result, detail.result);
    EXPECT_EQ(code, detail.code);
}

} // namespace

class socketTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_com_util_;

#if defined(PLATFORM_LINUX)
    NiceMock<Mock_sys_socket> mock_sys_socket_;
    NiceMock<Mock_fcntl> mock_fcntl_;
    NiceMock<Mock_poll> mock_poll_;
#elif defined(PLATFORM_WINDOWS)
    NiceMock<Mock_winsock> mock_winsock_;
#endif /* PLATFORM_ */

    /* プラットフォームごとにモック対象が異なるため、呼び出し期待の検証はヘルパーへ限定する。 */
    bool verifyExpectations()
    {
#if defined(PLATFORM_LINUX)
        return Mock::VerifyAndClearExpectations(&mock_com_util_) &&
               Mock::VerifyAndClearExpectations(&mock_sys_socket_);
#elif defined(PLATFORM_WINDOWS)
        return Mock::VerifyAndClearExpectations(&mock_com_util_) &&
               Mock::VerifyAndClearExpectations(&mock_winsock_);
#endif /* PLATFORM_ */
    }

    /* 下位のクローズ API が呼び出されないことを期待する。 */
    void expectNoCloseCall()
    {
#if defined(PLATFORM_LINUX)
        EXPECT_CALL(mock_com_util_, com_util_close(_, _)).Times(0);
#elif defined(PLATFORM_WINDOWS)
        EXPECT_CALL(mock_winsock_, closesocket(_, _, _, _)).Times(0);
#endif /* PLATFORM_ */
    }

    /* 下位のシャットダウン API が呼び出されないことを期待する。 */
    void expectNoShutdownCall()
    {
#if defined(PLATFORM_LINUX)
        EXPECT_CALL(mock_sys_socket_, shutdown(_, _, _, _, _)).Times(0);
#elif defined(PLATFORM_WINDOWS)
        EXPECT_CALL(mock_winsock_, shutdown(_, _, _, _, _)).Times(0);
#endif /* PLATFORM_ */
    }

#if defined(PLATFORM_WINDOWS)
    void SetUp() override
    {
        ON_CALL(mock_com_util_, com_util_call_once)
            .WillByDefault([](com_util_once_flag *, com_util_once_fn function) { function(); });
        ON_CALL(mock_winsock_, WSAStartup).WillByDefault(Return(0));
        ON_CALL(mock_winsock_, WSACleanup).WillByDefault(Return(0));
        ON_CALL(mock_winsock_, WSAGetLastError).WillByDefault(Return(0));
    }

    void TearDown() override
    {
        com_util_internal_socket_cleanup();
    }
#endif /* PLATFORM_WINDOWS */
};

// ソケットの引数不正が拒否されることの確認
TEST_F(socketTest, open_rejects_invalid_arguments)
{
    // Arrange
    com_util_socket socket = kSocket;
    // 列挙範囲外の不正値を意図的に渡す (定数キャストは -Wconversion になるため変数経由)
    int invalid_kind_value = COM_UTIL_SOCKET_UDP + 1; // [状態] - 未定義の種別を表す値とする。

    // Pre-Assert

    // Act
    int rtc_null_output = com_util_socket_open(COM_UTIL_SOCKET_TCP, NULL, NULL); // [手順] - 出力先に NULL を指定する。
    int rtc_invalid_kind = com_util_socket_open(static_cast<com_util_socket_kind>(invalid_kind_value), &socket,
                                                NULL); // [手順] - 未定義の種別を指定する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_null_output); // [確認_異常系] - 出力先が NULL の com_util_socket_open の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid_kind); // [確認_異常系] - 未定義の種別を指定した com_util_socket_open の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_INVALID_SOCKET,
              socket); // [確認_異常系] - 不正な種別の指定時に出力先が無効値になること。
}

// ソケットが TCP と UDP の種別に応じて生成されることの確認
TEST_F(socketTest, open_returns_socket_for_each_kind)
{
    // Arrange
    com_util_socket tcp_socket = COM_UTIL_INVALID_SOCKET;
    com_util_socket udp_socket = COM_UTIL_INVALID_SOCKET;

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位のソケット生成 API が AF_INET と SOCK_STREAM で 1 回、AF_INET と SOCK_DGRAM で 1 回呼び出されること。
    // [Pre-Assert手順] - 下位のソケット生成 API から TCP に 7、UDP に 8 のハンドルを返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_socket_, socket(_, _, _, AF_INET, SOCK_STREAM, 0)).WillOnce(Return(7));
    EXPECT_CALL(mock_sys_socket_, socket(_, _, _, AF_INET, SOCK_DGRAM, 0)).WillOnce(Return(8));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, socket(_, _, _, AF_INET, SOCK_STREAM, 0)).WillOnce(Return((SOCKET)7));
    EXPECT_CALL(mock_winsock_, socket(_, _, _, AF_INET, SOCK_DGRAM, 0)).WillOnce(Return((SOCKET)8));
#endif /* PLATFORM_ */

    // Act
    int rtc_tcp = com_util_socket_open(COM_UTIL_SOCKET_TCP, &tcp_socket, NULL); // [手順] - TCP ソケットを生成する。
    int rtc_udp = com_util_socket_open(COM_UTIL_SOCKET_UDP, &udp_socket, NULL); // [手順] - UDP ソケットを生成する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              rtc_tcp); // [確認_正常系] - TCP を指定した com_util_socket_open の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_udp); // [確認_正常系] - UDP を指定した com_util_socket_open の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ((com_util_socket)7,
              tcp_socket); // [確認_正常系] - TCP ソケットのハンドルが返されること。
    EXPECT_EQ((com_util_socket)8,
              udp_socket); // [確認_正常系] - UDP ソケットのハンドルが返されること。
}

// OS のソケット生成失敗が通知されることの確認
TEST_F(socketTest, open_reports_socket_failure)
{
    // Arrange
    com_util_socket socket = kSocket;
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_異常系] - 下位のソケット生成 API が AF_INET と SOCK_STREAM を指定して 1 回呼び出されること。
    // [Pre-Assert手順] - 下位のソケット生成 API から失敗を返却し、失敗要因を EMFILE 相当として通知する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_socket_, socket(_, _, _, AF_INET, SOCK_STREAM, 0))
        .WillOnce(DoAll(Assign(&errno, EMFILE),
                        Return(-1)));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, socket(_, _, _, AF_INET, SOCK_STREAM, 0)).WillOnce(Return(INVALID_SOCKET));
    EXPECT_CALL(mock_winsock_, WSAGetLastError)
        .WillOnce(
            Return(WSAEMFILE));
#endif /* PLATFORM_ */

    // Act
    int actual_ret = com_util_socket_open(COM_UTIL_SOCKET_TCP, &socket, &detail); // [手順] - ソケット生成失敗を注入する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        actual_ret); // [確認_異常系] - ソケット生成失敗時の com_util_socket_open の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(COM_UTIL_INVALID_SOCKET,
              socket); // [確認_異常系] - ソケット生成失敗時に無効値が返されること。
    // [確認_異常系] - 詳細エラーに実行環境に応じた OS のエラー ドメインとエラー値が記録されること。
#if defined(PLATFORM_LINUX)
    expect_detail(detail, COM_UTIL_ERROR_DOMAIN_SOCKET_ERRNO, COM_UTIL_ERR_UNKNOWN, static_cast<unsigned long>(EMFILE));
#elif defined(PLATFORM_WINDOWS)
    expect_detail(detail, COM_UTIL_ERROR_DOMAIN_WINSOCK, COM_UTIL_ERR_UNKNOWN, static_cast<unsigned long>(WSAEMFILE));
#endif /* PLATFORM_ */
}

#if defined(PLATFORM_WINDOWS)
// Winsock の初期化失敗がソケット生成へ伝播することの確認
TEST_F(socketTest, open_propagates_startup_failure)
{
    // Arrange
    com_util_socket socket = kSocket;
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_異常系] - WSAStartup が 1 回呼び出されること。
    // [Pre-Assert手順] - WSAStartup から WSASYSNOTREADY を返却する。
    EXPECT_CALL(mock_winsock_, WSAStartup(_, _, _, _, _))
        .WillOnce(Return(WSASYSNOTREADY));

    // Act
    int actual_ret = com_util_socket_open(COM_UTIL_SOCKET_TCP, &socket,
                                   &detail); // [手順] - 初期化失敗を注入してソケットを生成する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              actual_ret); // [確認_異常系] - 初期化失敗時の com_util_socket_open の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(COM_UTIL_INVALID_SOCKET,
              socket); // [確認_異常系] - 初期化失敗時に無効値が返されること。
    // [確認_異常系] - 詳細エラーに Winsock ドメインと OS のエラー値が記録されること。
    expect_detail(detail, COM_UTIL_ERROR_DOMAIN_WINSOCK, COM_UTIL_ERR_UNKNOWN,
                  static_cast<unsigned long>(WSASYSNOTREADY));
}
#endif /* PLATFORM_WINDOWS */

// 無効なソケットを閉じる場合に OS API が呼ばれないことの確認
TEST_F(socketTest, close_ignores_invalid_socket)
{
    // Arrange

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位のクローズ API が呼び出されないこと。
    expectNoCloseCall();

    // Act
    com_util_socket_close(COM_UTIL_INVALID_SOCKET); // [手順] - 無効なソケットを閉じる。

    // Assert
    EXPECT_TRUE(verifyExpectations());
    // [確認_正常系] - 無効なソケットの指定時に下位のクローズ API が呼び出されないこと。
}

// 有効なソケットを閉じる場合に OS API が呼ばれることの確認
TEST_F(socketTest, close_calls_os_close)
{
    // Arrange

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位のクローズ API がクローズ対象のソケットを引数として 1 回呼び出されること。
    // [Pre-Assert手順] - 下位のクローズ API から成功を返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_com_util_, com_util_close(kSocket, _)).WillOnce(Return(COM_UTIL_OK));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, closesocket(_, _, _, (SOCKET)kSocket)).WillOnce(Return(0));
#endif /* PLATFORM_ */

    // Act
    com_util_socket_close(kSocket); // [手順] - 有効なソケットを閉じる。

    // Assert
    EXPECT_TRUE(verifyExpectations());
    // [確認_正常系] - 下位のクローズ API への呼び出し期待が満たされること。
}

// クローズが呼び出し前の直前エラーを保存および復元することの確認
TEST_F(socketTest, close_preserves_last_error)
{
    // Arrange
    const com_util_error saved = {COM_UTIL_ERROR_DOMAIN_SOCKET_ERRNO, COM_UTIL_ERR_UNKNOWN,
                                  static_cast<unsigned long>(ECONNRESET)};
    com_util_error actual = {};

    com_util_error_set_last(&saved); // [状態] - 呼び出し前の直前エラーを ECONNRESET とする。

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位のクローズ API がクローズ対象のソケットを引数として 1 回呼び出されること。
    // [Pre-Assert手順] - 下位のクローズ API から直前エラーを EACCES へ書き換える。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_com_util_, com_util_close(kSocket, _))
        .WillOnce(
            [](int, com_util_error *)
            {
                const com_util_error overwritten = {COM_UTIL_ERROR_DOMAIN_ERRNO, COM_UTIL_ERR_PERMISSION_DENIED,
                                                    static_cast<unsigned long>(EACCES)};
                com_util_error_set_last(&overwritten);
                return COM_UTIL_OK;
            });
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, closesocket(_, _, _, (SOCKET)kSocket))
        .WillOnce(
            [](const char *, const int, const char *, SOCKET)
            {
                const com_util_error overwritten = {COM_UTIL_ERROR_DOMAIN_ERRNO, COM_UTIL_ERR_PERMISSION_DENIED,
                                                    static_cast<unsigned long>(EACCES)};
                com_util_error_set_last(&overwritten);
                return 0;
            });
#endif /* PLATFORM_ */

    // Act
    com_util_socket_close(kSocket); // [手順] - 直前エラーを書き換えるクローズ API を伴ってソケットを閉じる。

    // Assert
    com_util_error_get_last(&actual);
    EXPECT_EQ(saved.domain,
              actual.domain); // [確認_正常系] - クローズ後の直前エラーのドメインが呼び出し前と同じであること。
    EXPECT_EQ(saved.result,
              actual.result); // [確認_正常系] - クローズ後の直前エラーの結果コードが呼び出し前と同じであること。
    EXPECT_EQ(saved.code,
              actual.code); // [確認_正常系] - クローズ後の直前エラーのエラー値が呼び出し前と同じであること。

    // Cleanup
    com_util_error_clear_last();
}

// 無効なソケットのシャットダウンが無視されることの確認
TEST_F(socketTest, shutdown_ignores_invalid_socket)
{
    // Arrange

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位のシャットダウン API が呼び出されないこと。
    expectNoShutdownCall();

    // Act
    com_util_socket_shutdown(COM_UTIL_INVALID_SOCKET); // [手順] - 無効なソケットをシャットダウンする。

    // Assert
    EXPECT_TRUE(verifyExpectations());
    // [確認_正常系] - 無効なソケットの指定時に下位のシャットダウン API が呼び出されないこと。
}

// 有効なソケットのシャットダウンが OS API へ委譲されることの確認
TEST_F(socketTest, shutdown_calls_os_shutdown)
{
    // Arrange

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位のシャットダウン API が両方向の停止を指定して 1 回呼び出されること。
    // [Pre-Assert手順] - 下位のシャットダウン API から成功を返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_socket_, shutdown(_, _, _, (int)kSocket, SHUT_RDWR)).WillOnce(Return(0));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, shutdown(_, _, _, (SOCKET)kSocket, SD_BOTH)).WillOnce(Return(0));
#endif /* PLATFORM_ */

    // Act
    com_util_socket_shutdown(kSocket); // [手順] - 有効なソケットをシャットダウンする。

    // Assert
    EXPECT_TRUE(verifyExpectations());
    // [確認_正常系] - 下位のシャットダウン API への呼び出し期待が満たされること。
}

// bind、listen、connect の引数不正が拒否されることの確認
TEST_F(socketTest, connection_operations_reject_invalid_arguments)
{
    // Arrange
    com_util_error detail = {};

    // Pre-Assert

    // Act
    int rtc_bind_socket = com_util_socket_bind(COM_UTIL_INVALID_SOCKET, &kEndpoint,
                                               &detail); // [手順] - bind のソケットに無効値を指定する。
    int rtc_bind_endpoint = com_util_socket_bind(kSocket, NULL, &detail); // [手順] - bind の端点に NULL を指定する。
    int rtc_listen_socket =
        com_util_socket_listen(COM_UTIL_INVALID_SOCKET, 1, &detail); // [手順] - listen のソケットに無効値を指定する。
    int rtc_listen_backlog =
        com_util_socket_listen(kSocket, -1, &detail); // [手順] - listen に負の待ち受け数を指定する。
    int rtc_connect_socket = com_util_socket_connect(COM_UTIL_INVALID_SOCKET, &kEndpoint,
                                                     &detail); // [手順] - connect のソケットに無効値を指定する。
    int rtc_connect_endpoint =
        com_util_socket_connect(kSocket, NULL, &detail); // [手順] - connect の端点に NULL を指定する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_bind_socket); // [確認_異常系] - 無効なソケットを指定した bind の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_bind_endpoint); // [確認_異常系] - NULL の端点を指定した bind の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_listen_socket); // [確認_異常系] - 無効なソケットを指定した listen の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_listen_backlog); // [確認_異常系] - 負の待ち受け数を指定した listen の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_connect_socket); // [確認_異常系] - 無効なソケットを指定した connect の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_connect_endpoint); // [確認_異常系] - NULL の端点を指定した connect の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// bind、listen、connect が成功することの確認
TEST_F(socketTest, connection_operations_succeed)
{
    // Arrange

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の bind と connect が対象のソケットを引数として 1 回ずつ、listen が既定と 3 の待ち受けキュー長で 1 回ずつ呼び出されること。
    // [Pre-Assert手順] - 下位の bind、listen、connect の各 API から成功を返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_socket_, bind(_, _, _, (int)kSocket, _, _)).WillOnce(Return(0));
    EXPECT_CALL(mock_sys_socket_, listen(_, _, _, (int)kSocket, SOMAXCONN)).WillOnce(Return(0));
    EXPECT_CALL(mock_sys_socket_, listen(_, _, _, (int)kSocket, 3)).WillOnce(Return(0));
    EXPECT_CALL(mock_sys_socket_, connect(_, _, _, (int)kSocket, _, _)).WillOnce(Return(0));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, bind(_, _, _, (SOCKET)kSocket, _, _)).WillOnce(Return(0));
    EXPECT_CALL(mock_winsock_, listen(_, _, _, (SOCKET)kSocket, SOMAXCONN)).WillOnce(Return(0));
    EXPECT_CALL(mock_winsock_, listen(_, _, _, (SOCKET)kSocket, 3)).WillOnce(Return(0));
    EXPECT_CALL(mock_winsock_, connect(_, _, _, (SOCKET)kSocket, _, _)).WillOnce(Return(0));
#endif /* PLATFORM_ */

    // Act
    int rtc_bind = com_util_socket_bind(kSocket, &kEndpoint, NULL); // [手順] - bind を成功させる。
    int rtc_listen_default =
        com_util_socket_listen(kSocket, COM_UTIL_SOCKET_BACKLOG_DEFAULT, NULL); // [手順] - 既定値で listen を呼び出す。
    int rtc_listen_explicit =
        com_util_socket_listen(kSocket, 3, NULL); // [手順] - 明示した待ち受け数で listen を呼び出す。
    int rtc_connect = com_util_socket_connect(kSocket, &kEndpoint, NULL); // [手順] - connect を成功させる。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              rtc_bind); // [確認_正常系] - bind の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_listen_default); // [確認_正常系] - 既定値の listen の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_listen_explicit); // [確認_正常系] - 明示値の listen の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_connect); // [確認_正常系] - connect の戻り値が COM_UTIL_OK であること。
}

// bind、listen、connect の OS 失敗が通知されることの確認
TEST_F(socketTest, connection_operations_report_os_failures)
{
    // Arrange
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_異常系] - 下位の bind、listen、connect の各 API が対象のソケットを引数として 1 回ずつ呼び出されること。
    // [Pre-Assert手順] - 下位の bind、listen、connect の各 API から失敗を返却し、それぞれの失敗要因を通知する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_socket_, bind(_, _, _, (int)kSocket, _, _))
        .WillOnce(DoAll(Assign(&errno, EADDRINUSE), Return(-1)));
    EXPECT_CALL(mock_sys_socket_, listen(_, _, _, (int)kSocket, _)).WillOnce(DoAll(Assign(&errno, EIO), Return(-1)));
    EXPECT_CALL(mock_sys_socket_, connect(_, _, _, (int)kSocket, _, _))
        .WillOnce(DoAll(Assign(&errno, ECONNREFUSED), Return(-1)));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, bind(_, _, _, (SOCKET)kSocket, _, _)).WillOnce(Return(SOCKET_ERROR));
    EXPECT_CALL(mock_winsock_, listen(_, _, _, (SOCKET)kSocket, _)).WillOnce(Return(SOCKET_ERROR));
    EXPECT_CALL(mock_winsock_, connect(_, _, _, (SOCKET)kSocket, _, _)).WillOnce(Return(SOCKET_ERROR));
    EXPECT_CALL(mock_winsock_, WSAGetLastError)
        .WillOnce(Return(WSAEADDRINUSE))
        .WillOnce(Return(WSAENETDOWN))
        .WillOnce(Return(WSAECONNREFUSED));
#endif /* PLATFORM_ */

    // Act
    int rtc_bind = com_util_socket_bind(kSocket, &kEndpoint, &detail);       // [手順] - bind の失敗を注入する。
    int rtc_listen = com_util_socket_listen(kSocket, 1, &detail);            // [手順] - listen の失敗を注入する。
    int rtc_connect = com_util_socket_connect(kSocket, &kEndpoint, &detail); // [手順] - connect の失敗を注入する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_bind); // [確認_異常系] - bind の OS 失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_listen); // [確認_異常系] - listen の OS 失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_connect); // [確認_異常系] - connect の OS 失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
}

// 非ブロッキング connect の継続状態がプラットフォーム共通の結果コードになることの確認
TEST_F(socketTest, connect_reports_in_progress_for_nonblocking_completion)
{
    // Arrange
    com_util_error detail = {};

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_socket_, connect(_, _, _, (int)kSocket, _, _))
        .WillOnce(DoAll(Assign(&errno, EINPROGRESS), Return(-1))); // [Pre-Assert手順] - connect から EINPROGRESS を返却する。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, connect(_, _, _, (SOCKET)kSocket, _, _))
        .WillOnce(Return(SOCKET_ERROR)); // [Pre-Assert手順] - connect から SOCKET_ERROR を返却する。
    EXPECT_CALL(mock_winsock_, WSAGetLastError)
        .WillOnce(Return(WSAEWOULDBLOCK)); // [Pre-Assert手順] - WSAGetLastError から WSAEWOULDBLOCK を返却する。
#endif /* PLATFORM_ */

    // Act
    int result = com_util_socket_connect(kSocket, &kEndpoint,
                                         &detail); // [手順] - 非ブロッキング connect の継続状態を処理する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_IN_PROGRESS,
              result); // [確認_正常系] - com_util_socket_connect の戻り値が COM_UTIL_ERR_IN_PROGRESS であること。
#if defined(PLATFORM_LINUX)
    EXPECT_EQ(COM_UTIL_CAUSE_IN_PROGRESS,
              com_util_error_get_cause(&detail)); // [確認_正常系] - Linux の詳細要因が IN_PROGRESS であること。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_EQ(COM_UTIL_CAUSE_WOULD_BLOCK,
              com_util_error_get_cause(&detail)); // [確認_正常系] - Windows の生の詳細要因が WOULD_BLOCK であること。
#endif /* PLATFORM_ */
}

// accept が無効な引数を拒否することの確認
TEST_F(socketTest, accept_rejects_invalid_arguments)
{
    // Arrange
    com_util_socket accepted = kSocket;

    // Pre-Assert

    // Act
    int rtc_socket = com_util_socket_accept(COM_UTIL_INVALID_SOCKET, NULL, &accepted,
                                            NULL); // [手順] - accept の待ち受けソケットに無効値を指定する。
    int rtc_output = com_util_socket_accept(kSocket, NULL, NULL, NULL); // [手順] - accept の出力先に NULL を指定する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_socket); // [確認_異常系] - 無効なソケットを指定した accept の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_output); // [確認_異常系] - 出力先が NULL の accept の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// accept が接続元端点と新しいソケットを返すことの確認
TEST_F(socketTest, accept_returns_peer_and_socket)
{
    // Arrange
    com_util_ipv4_endpoint peer = {};
    com_util_socket accepted = COM_UTIL_INVALID_SOCKET;

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の accept API が待ち受けソケットを引数として呼び出されること。
    // [Pre-Assert手順] - 下位の accept API から接続元の端点と新しいソケットのハンドルを返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_socket_, accept(_, _, _, (int)kSocket, _, _))
        .WillRepeatedly(
            [](const char *, const int, const char *, int, struct sockaddr *address, socklen_t *length)
            {
                struct sockaddr_in *native = reinterpret_cast<struct sockaddr_in *>(address);
                native->sin_addr.s_addr = COM_UTIL_IPV4_ADDR_LOOPBACK;
                native->sin_port = com_util_hton16((uint16_t)54321U);
                *length = (socklen_t)sizeof(*native);
                return 8;
            });
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, accept(_, _, _, (SOCKET)kSocket, _, _))
        .WillRepeatedly(
            [](const char *, const int, const char *, SOCKET, struct sockaddr *address, int *length)
            {
                SOCKADDR_IN *native = reinterpret_cast<SOCKADDR_IN *>(address);
                native->sin_addr.S_un.S_addr = COM_UTIL_IPV4_ADDR_LOOPBACK;
                native->sin_port = com_util_hton16((uint16_t)54321U);
                *length = (int)sizeof(*native);
                return (SOCKET)8;
            });
#endif /* PLATFORM_ */

    // Act
    int actual_ret = com_util_socket_accept(kSocket, &peer, &accepted, NULL); // [手順] - 接続を受け付ける。
    int rtc_without_peer = com_util_socket_accept(kSocket, NULL, &accepted,
                                                  NULL); // [手順] - 接続元端点の出力先を NULL にして接続を受け付ける。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              actual_ret); // [確認_正常系] - accept の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_without_peer); // [確認_正常系] - 接続元端点を要求しない accept の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ((com_util_socket)8,
              accepted); // [確認_正常系] - 受け付けたソケットが返されること。
    EXPECT_EQ(COM_UTIL_IPV4_ADDR_LOOPBACK,
              peer.address); // [確認_正常系] - 接続元アドレスが返されること。
    EXPECT_EQ(com_util_hton16((uint16_t)54321U),
              peer.port); // [確認_正常系] - 接続元ポートが返されること。
}

// accept の OS 失敗が通知されることの確認
TEST_F(socketTest, accept_reports_os_failure)
{
    // Arrange
    com_util_socket accepted = kSocket;
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_異常系] - 下位の accept API が待ち受けソケットを引数として 1 回呼び出されること。
    // [Pre-Assert手順] - 下位の accept API から失敗を返却し、失敗要因を通知する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_socket_, accept(_, _, _, (int)kSocket, _, _)).WillOnce(DoAll(Assign(&errno, EIO), Return(-1)));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, accept(_, _, _, (SOCKET)kSocket, _, _)).WillOnce(Return(INVALID_SOCKET));
    EXPECT_CALL(mock_winsock_, WSAGetLastError).WillOnce(Return(WSAENETDOWN));
#endif /* PLATFORM_ */

    // Act
    int actual_ret = com_util_socket_accept(kSocket, NULL, &accepted, &detail); // [手順] - accept の失敗を注入する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              actual_ret); // [確認_異常系] - accept の OS 失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(COM_UTIL_INVALID_SOCKET,
              accepted); // [確認_異常系] - accept の失敗時に無効値が返されること。
}

// 保留エラーの取得結果が分類されることの確認
TEST_F(socketTest, pending_error_reports_empty_pending_and_failure)
{
    // Arrange
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の getsockopt API が SOL_SOCKET と SO_ERROR を指定して呼び出されること。
    // [Pre-Assert手順] - 下位の getsockopt API から、保留エラーなし、保留エラーあり、取得失敗の順に応答する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_socket_, getsockopt(_, _, _, (int)kSocket, SOL_SOCKET, SO_ERROR, _, _))
        .WillOnce(
            [](const char *, const int, const char *, int, int, int, void *value, socklen_t *)
            {
                *static_cast<int *>(value) = 0;
                return 0;
            })
        .WillOnce(
            [](const char *, const int, const char *, int, int, int, void *value, socklen_t *)
            {
                *static_cast<int *>(value) = ECONNREFUSED;
                return 0;
            })
        .WillOnce(DoAll(Assign(&errno, EBADF), Return(-1)));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, getsockopt(_, _, _, (SOCKET)kSocket, SOL_SOCKET, SO_ERROR, _, _))
        .WillOnce(
            [](const char *, const int, const char *, SOCKET, int, int, char *value, int *)
            {
                *reinterpret_cast<int *>(value) = 0;
                return 0;
            })
        .WillOnce(
            [](const char *, const int, const char *, SOCKET, int, int, char *value, int *)
            {
                *reinterpret_cast<int *>(value) = WSAECONNREFUSED;
                return 0;
            })
        .WillOnce(Return(SOCKET_ERROR));
    EXPECT_CALL(mock_winsock_, WSAGetLastError).WillOnce(Return(WSAENOTSOCK));
#endif /* PLATFORM_ */

    // Act
    int rtc_invalid = com_util_socket_get_pending_error(COM_UTIL_INVALID_SOCKET,
                                                        &detail); // [手順] - 無効なソケットの保留エラーを取得する。
    int rtc_empty = com_util_socket_get_pending_error(kSocket, &detail);   // [手順] - 保留エラーがない状態を取得する。
    int rtc_pending = com_util_socket_get_pending_error(kSocket, &detail); // [手順] - 保留エラーがある状態を取得する。
    int rtc_failure = com_util_socket_get_pending_error(kSocket, &detail); // [手順] - getsockopt の失敗を注入する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid); // [確認_異常系] - 無効なソケットの保留エラー取得戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_empty); // [確認_正常系] - 保留エラーがない場合の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_pending); // [確認_異常系] - 保留エラーがある場合の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_failure); // [確認_異常系] - getsockopt 失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
}

// 非ブロッキング設定の引数不正と OS 失敗が処理されることの確認
TEST_F(socketTest, nonblocking_reports_invalid_and_os_failure)
{
    // Arrange
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の非ブロッキング設定 API が対象のソケットを引数として呼び出されること。
    // [Pre-Assert手順] - 下位の非ブロッキング設定 API から、有効化の成功、無効化の成功、失敗の順に応答する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_fcntl_, fcntl(_, _, _, (int)kSocket, F_GETFL, 0))
        .WillOnce(Return(O_RDONLY))
        .WillOnce(Return(O_NONBLOCK))
        .WillOnce(DoAll(Assign(&errno, EBADF), Return(-1)));
    EXPECT_CALL(mock_fcntl_, fcntl(_, _, _, (int)kSocket, F_SETFL, O_RDONLY | O_NONBLOCK)).WillOnce(Return(0));
    EXPECT_CALL(mock_fcntl_, fcntl(_, _, _, (int)kSocket, F_SETFL, 0)).WillOnce(Return(0));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, ioctlsocket(_, _, _, (SOCKET)kSocket, FIONBIO, _))
        .WillOnce(
            [](const char *, const int, const char *, SOCKET, long, u_long *mode)
            {
                EXPECT_EQ(1UL, *mode);
                return 0;
            })
        .WillOnce(
            [](const char *, const int, const char *, SOCKET, long, u_long *mode)
            {
                EXPECT_EQ(0UL, *mode);
                return 0;
            })
        .WillOnce(Return(SOCKET_ERROR));
    EXPECT_CALL(mock_winsock_, WSAGetLastError).WillOnce(Return(WSAENETDOWN));
#endif /* PLATFORM_ */

    // Act
    int rtc_invalid =
        com_util_socket_set_nonblocking(COM_UTIL_INVALID_SOCKET, 1, &detail); // [手順] - 無効なソケットを指定する。
    int rtc_enable = com_util_socket_set_nonblocking(kSocket, 1, &detail);    // [手順] - 非ブロッキングを有効にする。
    int rtc_disable = com_util_socket_set_nonblocking(kSocket, 0, &detail);   // [手順] - 非ブロッキングを無効にする。
    int rtc_failure =
        com_util_socket_set_nonblocking(kSocket, 1, &detail); // [手順] - 非ブロッキング設定の失敗を注入する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid); // [確認_異常系] - 無効なソケットを指定した戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_enable); // [確認_正常系] - 非ブロッキング有効化の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_disable); // [確認_正常系] - 非ブロッキング無効化の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_failure); // [確認_異常系] - 非ブロッキング設定失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
}

#if defined(PLATFORM_LINUX)
// F_SETFL の失敗が通知されることの確認
TEST_F(socketTest, nonblocking_reports_set_failure)
{
    // Arrange
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_異常系] - 下位の非ブロッキング設定 API が対象のソケットを引数として呼び出されること。
    // [Pre-Assert手順] - 下位の非ブロッキング設定 API から、現在の設定の取得に成功したのち設定の反映で失敗を返却する。
    EXPECT_CALL(mock_fcntl_, fcntl(_, _, _, (int)kSocket, F_GETFL, 0)).WillOnce(Return(O_RDONLY));
    EXPECT_CALL(mock_fcntl_, fcntl(_, _, _, (int)kSocket, F_SETFL, O_NONBLOCK))
        .WillOnce(
            DoAll(Assign(&errno, EIO),
                  Return(-1))); // [Pre-Assert確認_異常系] - F_SETFL が O_NONBLOCK を指定して 1 回呼び出されること。
                                // [Pre-Assert手順] - errno に EIO を設定し、-1 を返却する。

    // Act
    int actual_ret = com_util_socket_set_nonblocking(kSocket, 1, &detail); // [手順] - F_SETFL の失敗を注入する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        actual_ret); // [確認_異常系] - F_SETFL 失敗時の com_util_socket_set_nonblocking の戻り値が COM_UTIL_ERR_UNKNOWN であること。
}
#endif /* PLATFORM_LINUX */

// ソケット オプションの設定が成功することの確認
TEST_F(socketTest, socket_options_succeed)
{
    // Arrange
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の setsockopt API が SO_REUSEADDR と SO_BROADCAST を有効と無効で 1 回ずつ、IP_MULTICAST_IF、IP_ADD_MEMBERSHIP、IP_DROP_MEMBERSHIP を 1 回ずつ指定して呼び出されること。
    // [Pre-Assert手順] - 下位の setsockopt API から成功を返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_socket_, setsockopt(_, _, _, (int)kSocket, SOL_SOCKET, SO_REUSEADDR, _, _))
        .WillOnce(Return(0))
        .WillOnce(Return(0));
    EXPECT_CALL(mock_sys_socket_, setsockopt(_, _, _, (int)kSocket, SOL_SOCKET, SO_BROADCAST, _, _))
        .WillOnce(Return(0))
        .WillOnce(Return(0));
    EXPECT_CALL(mock_sys_socket_, setsockopt(_, _, _, (int)kSocket, IPPROTO_IP, IP_MULTICAST_IF, _, _))
        .WillOnce(Return(0));
    EXPECT_CALL(mock_sys_socket_, setsockopt(_, _, _, (int)kSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, _, _))
        .WillOnce(Return(0));
    EXPECT_CALL(mock_sys_socket_, setsockopt(_, _, _, (int)kSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, _, _))
        .WillOnce(Return(0));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, setsockopt(_, _, _, (SOCKET)kSocket, SOL_SOCKET, SO_REUSEADDR, _, _))
        .WillOnce(Return(0))
        .WillOnce(Return(0));
    EXPECT_CALL(mock_winsock_, setsockopt(_, _, _, (SOCKET)kSocket, SOL_SOCKET, SO_BROADCAST, _, _))
        .WillOnce(Return(0))
        .WillOnce(Return(0));
    EXPECT_CALL(mock_winsock_, setsockopt(_, _, _, (SOCKET)kSocket, IPPROTO_IP, IP_MULTICAST_IF, _, _))
        .WillOnce(Return(0));
    EXPECT_CALL(mock_winsock_, setsockopt(_, _, _, (SOCKET)kSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, _, _))
        .WillOnce(Return(0));
    EXPECT_CALL(mock_winsock_, setsockopt(_, _, _, (SOCKET)kSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, _, _))
        .WillOnce(Return(0));
#endif /* PLATFORM_ */

    // Act
    int rtc_reuse = com_util_socket_set_reuse_address(kSocket, 1, &detail); // [手順] - アドレス再利用を有効にする。
    int rtc_reuse_disabled =
        com_util_socket_set_reuse_address(kSocket, 0, &detail);             // [手順] - アドレス再利用を無効にする。
    int rtc_broadcast = com_util_socket_set_broadcast(kSocket, 0, &detail); // [手順] - ブロードキャストを無効にする。
    int rtc_broadcast_enabled =
        com_util_socket_set_broadcast(kSocket, 1, &detail); // [手順] - ブロードキャストを有効にする。
    int rtc_interface = com_util_socket_set_multicast_interface(
        kSocket, COM_UTIL_IPV4_ADDR_LOOPBACK, &detail); // [手順] - マルチキャスト インターフェースを設定する。
    int rtc_join = com_util_socket_join_multicast_group(kSocket, COM_UTIL_IPV4_ADDR_LOOPBACK, COM_UTIL_IPV4_ADDR_ANY,
                                                        &detail); // [手順] - マルチキャスト グループへ参加する。
    int rtc_leave = com_util_socket_leave_multicast_group(
        kSocket, COM_UTIL_IPV4_ADDR_LOOPBACK, COM_UTIL_IPV4_ADDR_ANY, &detail); // [手順] - マルチキャスト グループから離脱する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              rtc_reuse); // [確認_正常系] - アドレス再利用設定の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_reuse_disabled); // [確認_正常系] - アドレス再利用無効化の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_broadcast); // [確認_正常系] - ブロードキャスト設定の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_broadcast_enabled); // [確認_正常系] - ブロードキャスト有効化の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_interface); // [確認_正常系] - マルチキャスト インターフェース設定の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_join); // [確認_正常系] - マルチキャスト参加の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_leave); // [確認_正常系] - マルチキャスト離脱の戻り値が COM_UTIL_OK であること。
}

// ソケット オプションの引数不正と OS 失敗が処理されることの確認
TEST_F(socketTest, socket_options_report_invalid_and_os_failure)
{
    // Arrange
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_異常系] - 下位の setsockopt API が SO_REUSEADDR、IP_MULTICAST_IF、IP_ADD_MEMBERSHIP、IP_DROP_MEMBERSHIP を指定して 1 回ずつ呼び出されること。
    // [Pre-Assert手順] - 下位の setsockopt API から失敗を返却し、それぞれの失敗要因を通知する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_socket_, setsockopt(_, _, _, (int)kSocket, SOL_SOCKET, SO_REUSEADDR, _, _))
        .WillOnce(DoAll(Assign(&errno, ENOPROTOOPT), Return(-1)));
    EXPECT_CALL(mock_sys_socket_, setsockopt(_, _, _, (int)kSocket, IPPROTO_IP, IP_MULTICAST_IF, _, _))
        .WillOnce(DoAll(Assign(&errno, ENODEV), Return(-1)));
    EXPECT_CALL(mock_sys_socket_, setsockopt(_, _, _, (int)kSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, _, _))
        .WillOnce(DoAll(Assign(&errno, ENODEV), Return(-1)));
    EXPECT_CALL(mock_sys_socket_, setsockopt(_, _, _, (int)kSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, _, _))
        .WillOnce(DoAll(Assign(&errno, ENODEV), Return(-1)));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, setsockopt(_, _, _, (SOCKET)kSocket, SOL_SOCKET, SO_REUSEADDR, _, _))
        .WillOnce(Return(SOCKET_ERROR));
    EXPECT_CALL(mock_winsock_, setsockopt(_, _, _, (SOCKET)kSocket, IPPROTO_IP, IP_MULTICAST_IF, _, _))
        .WillOnce(Return(SOCKET_ERROR));
    EXPECT_CALL(mock_winsock_, setsockopt(_, _, _, (SOCKET)kSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, _, _))
        .WillOnce(Return(SOCKET_ERROR));
    EXPECT_CALL(mock_winsock_, setsockopt(_, _, _, (SOCKET)kSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, _, _))
        .WillOnce(Return(SOCKET_ERROR));
    EXPECT_CALL(mock_winsock_, WSAGetLastError)
        .WillOnce(Return(WSAENOPROTOOPT))
        .WillOnce(Return(WSAENETDOWN))
        .WillOnce(Return(WSAENETDOWN))
        .WillOnce(Return(WSAENETDOWN));
#endif /* PLATFORM_ */

    // Act
    int rtc_reuse_invalid = com_util_socket_set_reuse_address(COM_UTIL_INVALID_SOCKET, 1,
                                                              &detail); // [手順] - 無効なソケットで再利用を設定する。
    int rtc_broadcast_invalid = com_util_socket_set_broadcast(
        COM_UTIL_INVALID_SOCKET, 1, &detail); // [手順] - 無効なソケットでブロードキャストを設定する。
    int rtc_interface_invalid =
        com_util_socket_set_multicast_interface(COM_UTIL_INVALID_SOCKET, COM_UTIL_IPV4_ADDR_ANY,
                                                &detail); // [手順] - 無効なソケットでインターフェースを設定する。
    int rtc_join_invalid = com_util_socket_join_multicast_group(
        COM_UTIL_INVALID_SOCKET, COM_UTIL_IPV4_ADDR_LOOPBACK, COM_UTIL_IPV4_ADDR_ANY,
        &detail); // [手順] - 無効なソケットでグループ参加を設定する。
    int rtc_leave_invalid = com_util_socket_leave_multicast_group(
        COM_UTIL_INVALID_SOCKET, COM_UTIL_IPV4_ADDR_LOOPBACK, COM_UTIL_IPV4_ADDR_ANY,
        &detail); // [手順] - 無効なソケットでグループ離脱を設定する。
    int rtc_failure =
        com_util_socket_set_reuse_address(kSocket, 1, &detail); // [手順] - オプション設定の失敗を注入する。
    int rtc_interface_failure = com_util_socket_set_multicast_interface(
        kSocket, COM_UTIL_IPV4_ADDR_LOOPBACK, &detail); // [手順] - インターフェース設定の失敗を注入する。
    int rtc_join_failure =
        com_util_socket_join_multicast_group(kSocket, COM_UTIL_IPV4_ADDR_LOOPBACK, COM_UTIL_IPV4_ADDR_ANY,
                                             &detail); // [手順] - グループ参加の失敗を注入する。
    int rtc_leave_failure =
        com_util_socket_leave_multicast_group(kSocket, COM_UTIL_IPV4_ADDR_LOOPBACK, COM_UTIL_IPV4_ADDR_ANY,
                                              &detail); // [手順] - グループ離脱の失敗を注入する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_reuse_invalid); // [確認_異常系] - 無効なソケットを指定した再利用設定の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_broadcast_invalid); // [確認_異常系] - 無効なソケットを指定したブロードキャスト設定の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_interface_invalid); // [確認_異常系] - 無効なソケットを指定したインターフェース設定の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_join_invalid); // [確認_異常系] - 無効なソケットを指定したグループ参加の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_failure); // [確認_異常系] - オプション設定失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        rtc_interface_failure); // [確認_異常系] - インターフェース設定失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_join_failure); // [確認_異常系] - グループ参加失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_leave_invalid); // [確認_異常系] - 無効なソケットを指定したグループ離脱の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        rtc_leave_failure); // [確認_異常系] - グループ離脱失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
}

// 送受信の引数不正、成功、失敗が処理されることの確認
TEST_F(socketTest, send_and_recv_report_results)
{
    // Arrange
    char buffer[4] = {};
    size_t sent = 99U;
    size_t received = 99U;
    size_t sent_failure = 99U;
    size_t received_failure = 99U;
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の send が MSG_NOSIGNAL と 4 バイト、recv が 4 バイトの要求で 2 回ずつ呼び出されること。
    // [Pre-Assert手順] - 下位の send から 3 バイト送信ののち失敗、recv から 2 バイト受信ののち失敗を返却し、それぞれの失敗要因を通知する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_socket_, send(_, _, _, (int)kSocket, _, 4U, MSG_NOSIGNAL))
        .WillOnce(Return((ssize_t)3))
        .WillOnce(DoAll(Assign(&errno, EPIPE), Return((ssize_t)-1)));
    EXPECT_CALL(mock_sys_socket_, recv(_, _, _, (int)kSocket, _, 4U, 0))
        .WillOnce(Return((ssize_t)2))
        .WillOnce(DoAll(Assign(&errno, ECONNRESET), Return((ssize_t)-1)));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, send(_, _, _, (SOCKET)kSocket, _, 4, 0))
        .WillOnce(Return(3))
        .WillOnce(Return(SOCKET_ERROR));
    EXPECT_CALL(mock_winsock_, recv(_, _, _, (SOCKET)kSocket, _, 4, 0))
        .WillOnce(Return(2))
        .WillOnce(Return(SOCKET_ERROR));
    EXPECT_CALL(mock_winsock_, WSAGetLastError).WillOnce(Return(WSAECONNRESET)).WillOnce(Return(WSAECONNRESET));
#endif /* PLATFORM_ */

    // Act
    int rtc_invalid_send = com_util_socket_send(COM_UTIL_INVALID_SOCKET, buffer, sizeof(buffer), &sent,
                                                &detail); // [手順] - 無効なソケットで送信する。
    int rtc_invalid_send_buffer = com_util_socket_send(kSocket, NULL, sizeof(buffer), &sent,
                                                       &detail); // [手順] - NULL の送信バッファーを指定する。
    int rtc_invalid_send_output = com_util_socket_send(kSocket, buffer, sizeof(buffer), NULL,
                                                       &detail); // [手順] - NULL の送信バイト数出力先を指定する。
    int rtc_invalid_send_length = com_util_socket_send(kSocket, buffer, COM_UTIL_SOCKET_MAX_TRANSFER + (size_t)1U,
                                                       &sent, &detail); // [手順] - 最大転送量を超える送信長を指定する。
    int rtc_invalid_recv_socket = com_util_socket_recv(COM_UTIL_INVALID_SOCKET, buffer, sizeof(buffer), &received,
                                                       &detail); // [手順] - 無効なソケットで受信する。
    int rtc_invalid_recv = com_util_socket_recv(kSocket, NULL, sizeof(buffer), &received,
                                                &detail); // [手順] - NULL の受信バッファーを指定する。
    int rtc_invalid_recv_output = com_util_socket_recv(kSocket, buffer, sizeof(buffer), NULL,
                                                       &detail); // [手順] - NULL の受信バイト数出力先を指定する。
    int rtc_invalid_recv_length =
        com_util_socket_recv(kSocket, buffer, COM_UTIL_SOCKET_MAX_TRANSFER + (size_t)1U, &received,
                             &detail); // [手順] - 最大転送量を超える受信長を指定する。
    int rtc_send =
        com_util_socket_send(kSocket, buffer, sizeof(buffer), &sent, &detail); // [手順] - 送信成功を注入する。
    int rtc_recv =
        com_util_socket_recv(kSocket, buffer, sizeof(buffer), &received, &detail); // [手順] - 受信成功を注入する。
    int rtc_send_failure =
        com_util_socket_send(kSocket, buffer, sizeof(buffer), &sent_failure, &detail); // [手順] - 送信失敗を注入する。
    int rtc_recv_failure = com_util_socket_recv(kSocket, buffer, sizeof(buffer), &received_failure,
                                                &detail); // [手順] - 受信失敗を注入する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid_send); // [確認_異常系] - 無効なソケットの送信戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid_send_buffer); // [確認_異常系] - NULL 送信バッファーの戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid_send_output); // [確認_異常系] - NULL 送信バイト数出力先の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid_send_length); // [確認_異常系] - 最大転送量超過の送信戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid_recv_socket); // [確認_異常系] - 無効なソケットの受信戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid_recv); // [確認_異常系] - NULL バッファーの受信戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid_recv_output); // [確認_異常系] - NULL 受信バイト数出力先の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid_recv_length); // [確認_異常系] - 最大転送量超過の受信戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_send); // [確認_正常系] - 送信成功時の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_recv); // [確認_正常系] - 受信成功時の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ((size_t)3,
              sent); // [確認_正常系] - com_util_socket_send が送信バイト数を返すこと。
    EXPECT_EQ((size_t)2,
              received); // [確認_正常系] - com_util_socket_recv が受信バイト数を返すこと。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_send_failure); // [確認_異常系] - 送信失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_recv_failure); // [確認_異常系] - 受信失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
}

// sendto と recvfrom が端点を変換して送受信することの確認
TEST_F(socketTest, datagram_operations_succeed)
{
    // Arrange
    char buffer[4] = {};
    size_t transferred = 0U;
    com_util_ipv4_endpoint peer = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の sendto と recvfrom の各 API が 4 バイトの要求で呼び出されること。
    // [Pre-Assert手順] - 下位の sendto から 4 バイトの送信を、recvfrom から送信元の端点と受信バイト数を返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_socket_, sendto(_, _, _, (int)kSocket, _, 4U, 0, _, _)).WillOnce(Return((ssize_t)4));
    EXPECT_CALL(mock_sys_socket_, recvfrom(_, _, _, (int)kSocket, _, 4U, 0, _, _))
        .WillRepeatedly(
            [](const char *, const int, const char *, int, void *, size_t, int, struct sockaddr *address, socklen_t *)
            {
                struct sockaddr_in *native = reinterpret_cast<struct sockaddr_in *>(address);
                native->sin_addr.s_addr = COM_UTIL_IPV4_ADDR_LOOPBACK;
                native->sin_port = kEndpoint.port;
                return (ssize_t)2;
            });
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, sendto(_, _, _, (SOCKET)kSocket, _, 4, 0, _, _)).WillOnce(Return(4));
    EXPECT_CALL(mock_winsock_, recvfrom(_, _, _, (SOCKET)kSocket, _, 4, 0, _, _))
        .WillRepeatedly(
            [](const char *, const int, const char *, SOCKET, char *, int, int, struct sockaddr *address, int *)
            {
                SOCKADDR_IN *native = reinterpret_cast<SOCKADDR_IN *>(address);
                native->sin_addr.S_un.S_addr = COM_UTIL_IPV4_ADDR_LOOPBACK;
                native->sin_port = kEndpoint.port;
                return 2;
            });
#endif /* PLATFORM_ */

    // Act
    int rtc_send = com_util_socket_sendto(kSocket, buffer, sizeof(buffer), &kEndpoint, &transferred,
                                          NULL); // [手順] - データグラムを送信する。
    int rtc_recv = com_util_socket_recvfrom(kSocket, buffer, sizeof(buffer), &peer, &transferred,
                                            NULL); // [手順] - データグラムを受信する。
    int rtc_recv_without_peer =
        com_util_socket_recvfrom(kSocket, buffer, sizeof(buffer), NULL, &transferred,
                                 NULL); // [手順] - 接続元端点の出力先を NULL にしてデータグラムを受信する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              rtc_send); // [確認_正常系] - sendto の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_recv); // [確認_正常系] - recvfrom の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(
        COM_UTIL_OK,
        rtc_recv_without_peer); // [確認_正常系] - 接続元端点を要求しない recvfrom の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ((size_t)2,
              transferred); // [確認_正常系] - recvfrom が受信バイト数を返すこと。
    EXPECT_EQ(kEndpoint.address,
              peer.address); // [確認_正常系] - recvfrom が接続元アドレスを返すこと。
    EXPECT_EQ(kEndpoint.port,
              peer.port); // [確認_正常系] - recvfrom が接続元ポートを返すこと。
}

// sendto と recvfrom の引数不正および OS 失敗が処理されることの確認
TEST_F(socketTest, datagram_operations_report_invalid_and_os_failure)
{
    // Arrange
    char buffer[4] = {};
    size_t transferred = 0U;
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_異常系] - 下位の sendto と recvfrom の各 API が 4 バイトの要求で 1 回ずつ呼び出されること。
    // [Pre-Assert手順] - 下位の sendto と recvfrom から失敗を返却し、それぞれの失敗要因を通知する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_socket_, sendto(_, _, _, (int)kSocket, _, 4U, 0, _, _))
        .WillOnce(DoAll(Assign(&errno, ENETUNREACH), Return((ssize_t)-1)));
    EXPECT_CALL(mock_sys_socket_, recvfrom(_, _, _, (int)kSocket, _, 4U, 0, _, _))
        .WillOnce(DoAll(Assign(&errno, ECONNREFUSED), Return((ssize_t)-1)));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, sendto(_, _, _, (SOCKET)kSocket, _, 4, 0, _, _)).WillOnce(Return(SOCKET_ERROR));
    EXPECT_CALL(mock_winsock_, recvfrom(_, _, _, (SOCKET)kSocket, _, 4, 0, _, _)).WillOnce(Return(SOCKET_ERROR));
    EXPECT_CALL(mock_winsock_, WSAGetLastError).WillOnce(Return(WSAENETUNREACH)).WillOnce(Return(WSAECONNREFUSED));
#endif /* PLATFORM_ */

    // Act
    int rtc_send_invalid_socket =
        com_util_socket_sendto(COM_UTIL_INVALID_SOCKET, buffer, sizeof(buffer), &kEndpoint, &transferred,
                               &detail); // [手順] - 無効なソケットで sendto を呼び出す。
    int rtc_send_invalid_buffer =
        com_util_socket_sendto(kSocket, NULL, sizeof(buffer), &kEndpoint, &transferred,
                               &detail); // [手順] - NULL の送信バッファーで sendto を呼び出す。
    int rtc_send_invalid_output =
        com_util_socket_sendto(kSocket, buffer, sizeof(buffer), &kEndpoint, NULL,
                               &detail); // [手順] - NULL の送信バイト数出力先で sendto を呼び出す。
    int rtc_send_invalid_length =
        com_util_socket_sendto(kSocket, buffer, COM_UTIL_SOCKET_MAX_TRANSFER + (size_t)1U, &kEndpoint, &transferred,
                               &detail); // [手順] - 最大転送量を超える長さで sendto を呼び出す。
    int rtc_send_invalid = com_util_socket_sendto(kSocket, buffer, sizeof(buffer), NULL, &transferred,
                                                  &detail); // [手順] - sendto の端点に NULL を指定する。
    int rtc_recv_invalid_socket =
        com_util_socket_recvfrom(COM_UTIL_INVALID_SOCKET, buffer, sizeof(buffer), NULL, &transferred,
                                 &detail); // [手順] - 無効なソケットで recvfrom を呼び出す。
    int rtc_recv_invalid_buffer =
        com_util_socket_recvfrom(kSocket, NULL, sizeof(buffer), NULL, &transferred,
                                 &detail); // [手順] - NULL の受信バッファーで recvfrom を呼び出す。
    int rtc_recv_invalid_length =
        com_util_socket_recvfrom(kSocket, buffer, COM_UTIL_SOCKET_MAX_TRANSFER + (size_t)1U, NULL, &transferred,
                                 &detail); // [手順] - 最大転送量を超える長さで recvfrom を呼び出す。
    int rtc_recv_invalid = com_util_socket_recvfrom(kSocket, buffer, sizeof(buffer), NULL, NULL,
                                                    &detail); // [手順] - recvfrom の出力先に NULL を指定する。
    int rtc_send_failure = com_util_socket_sendto(kSocket, buffer, sizeof(buffer), &kEndpoint, &transferred,
                                                  &detail); // [手順] - sendto の失敗を注入する。
    int rtc_recv_failure = com_util_socket_recvfrom(kSocket, buffer, sizeof(buffer), NULL, &transferred,
                                                    &detail); // [手順] - recvfrom の失敗を注入する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_send_invalid_socket); // [確認_異常系] - 無効なソケットの sendto 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_send_invalid_buffer); // [確認_異常系] - NULL 送信バッファーの sendto 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_send_invalid_output); // [確認_異常系] - NULL 送信バイト数出力先の sendto 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_send_invalid_length); // [確認_異常系] - 最大転送量超過の sendto 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_send_invalid); // [確認_異常系] - NULL の端点を指定した sendto の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_recv_invalid_socket); // [確認_異常系] - 無効なソケットの recvfrom 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_recv_invalid_buffer); // [確認_異常系] - NULL 受信バッファーの recvfrom 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_recv_invalid_length); // [確認_異常系] - 最大転送量超過の recvfrom 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_recv_invalid); // [確認_異常系] - NULL の出力先を指定した recvfrom の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_send_failure); // [確認_異常系] - sendto 失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_recv_failure); // [確認_異常系] - recvfrom 失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
}

// send_all の全送信、部分送信、ゼロ送信、失敗が処理されることの確認
TEST_F(socketTest, send_all_reports_results)
{
    // Arrange
    const char buffer[4] = {'a', 'b', 'c', 'd'};
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の send API が MSG_NOSIGNAL と、未送信の残量に応じた 4 バイトまたは 2 バイトの要求で呼び出されること。
    // [Pre-Assert手順] - 下位の send から、全量送信、部分送信、0 バイト送信、失敗の順に応答する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_socket_, send(_, _, _, (int)kSocket, _, 4U, MSG_NOSIGNAL))
        .WillOnce(Return((ssize_t)4))
        .WillOnce(Return((ssize_t)2))
        .WillOnce(Return((ssize_t)0))
        .WillOnce(DoAll(Assign(&errno, EPIPE), Return((ssize_t)-1)));
    EXPECT_CALL(mock_sys_socket_, send(_, _, _, (int)kSocket, _, 2U, MSG_NOSIGNAL)).WillOnce(Return((ssize_t)2));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, send(_, _, _, (SOCKET)kSocket, _, 4, 0))
        .WillOnce(Return(4))
        .WillOnce(Return(2))
        .WillOnce(Return(0))
        .WillOnce(Return(SOCKET_ERROR));
    EXPECT_CALL(mock_winsock_, send(_, _, _, (SOCKET)kSocket, _, 2, 0)).WillOnce(Return(2));
    EXPECT_CALL(mock_winsock_, WSAGetLastError).WillOnce(Return(WSAECONNRESET));
#endif /* PLATFORM_ */

    // Act
    int rtc_empty = com_util_socket_send_all(kSocket, buffer, 0U, &detail); // [手順] - 長さ 0 のデータを送信する。
    int rtc_full =
        com_util_socket_send_all(kSocket, buffer, sizeof(buffer), &detail); // [手順] - 全送信成功を注入する。
    int rtc_partial =
        com_util_socket_send_all(kSocket, buffer, sizeof(buffer), &detail); // [手順] - 部分送信を注入する。
    int rtc_zero =
        com_util_socket_send_all(kSocket, buffer, sizeof(buffer), &detail); // [手順] - 0 バイト送信を注入する。
    int rtc_failure =
        com_util_socket_send_all(kSocket, buffer, sizeof(buffer), &detail); // [手順] - 送信失敗を注入する。
    int rtc_invalid = com_util_socket_send_all(COM_UTIL_INVALID_SOCKET, buffer, sizeof(buffer),
                                               &detail); // [手順] - 無効なソケットを指定する。
    int rtc_invalid_buffer =
        com_util_socket_send_all(kSocket, NULL, sizeof(buffer), &detail); // [手順] - NULL の送信バッファーを指定する。
    int rtc_invalid_length = com_util_socket_send_all(kSocket, buffer, COM_UTIL_SOCKET_MAX_TRANSFER + (size_t)1U,
                                                      &detail); // [手順] - 最大転送量を超える送信長を指定する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              rtc_empty); // [確認_正常系] - 長さ 0 の send_all の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_full); // [確認_正常系] - 全送信成功時の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_partial); // [確認_正常系] - 部分送信成功時の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_zero); // [確認_異常系] - 0 バイト送信時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_failure); // [確認_異常系] - 送信失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid); // [確認_異常系] - 無効なソケットの send_all の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid_buffer); // [確認_異常系] - NULL 送信バッファーの send_all 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid_length); // [確認_異常系] - 最大転送量超過の send_all 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// recv_all の全受信、部分受信、EOF、失敗が処理されることの確認
TEST_F(socketTest, recv_all_reports_results)
{
    // Arrange
    char buffer[4] = {};
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の recv API が、未受信の残量に応じて 4 バイトと 2 バイトの要求で呼び出されること。
    // [Pre-Assert手順] - 下位の recv から、全量受信、部分受信、0 バイト受信、失敗の順に応答する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_socket_, recv(_, _, _, (int)kSocket, _, 4U, 0))
        .WillOnce(Return((ssize_t)4))
        .WillOnce(Return((ssize_t)2))
        .WillOnce(Return((ssize_t)0))
        .WillOnce(DoAll(Assign(&errno, ECONNRESET), Return((ssize_t)-1)));
    EXPECT_CALL(mock_sys_socket_, recv(_, _, _, (int)kSocket, _, 2U, 0)).WillOnce(Return((ssize_t)2));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, recv(_, _, _, (SOCKET)kSocket, _, 4, 0))
        .WillOnce(Return(4))
        .WillOnce(Return(2))
        .WillOnce(Return(0))
        .WillOnce(Return(SOCKET_ERROR));
    EXPECT_CALL(mock_winsock_, recv(_, _, _, (SOCKET)kSocket, _, 2, 0)).WillOnce(Return(2));
    EXPECT_CALL(mock_winsock_, WSAGetLastError).WillOnce(Return(WSAECONNRESET));
#endif /* PLATFORM_ */

    // Act
    int rtc_empty = com_util_socket_recv_all(kSocket, buffer, 0U, &detail); // [手順] - 長さ 0 のデータを受信する。
    int rtc_full =
        com_util_socket_recv_all(kSocket, buffer, sizeof(buffer), &detail); // [手順] - 全受信成功を注入する。
    int rtc_partial =
        com_util_socket_recv_all(kSocket, buffer, sizeof(buffer), &detail);           // [手順] - 部分受信を注入する。
    int rtc_eof = com_util_socket_recv_all(kSocket, buffer, sizeof(buffer), &detail); // [手順] - EOF を注入する。
    int rtc_failure =
        com_util_socket_recv_all(kSocket, buffer, sizeof(buffer), &detail); // [手順] - 受信失敗を注入する。
    int rtc_invalid = com_util_socket_recv_all(COM_UTIL_INVALID_SOCKET, buffer, sizeof(buffer),
                                               &detail); // [手順] - 無効なソケットを指定する。
    int rtc_invalid_buffer =
        com_util_socket_recv_all(kSocket, NULL, sizeof(buffer), &detail); // [手順] - NULL の受信バッファーを指定する。
    int rtc_invalid_length = com_util_socket_recv_all(kSocket, buffer, COM_UTIL_SOCKET_MAX_TRANSFER + (size_t)1U,
                                                      &detail); // [手順] - 最大転送量を超える受信長を指定する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              rtc_empty); // [確認_正常系] - 長さ 0 の recv_all の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_full); // [確認_正常系] - 全受信成功時の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_partial); // [確認_正常系] - 部分受信成功時の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_ERR_EOF,
              rtc_eof); // [確認_異常系] - EOF 時の recv_all の戻り値が COM_UTIL_ERR_EOF であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_failure); // [確認_異常系] - 受信失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid); // [確認_異常系] - 無効なソケットの recv_all の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid_buffer); // [確認_異常系] - NULL 受信バッファーの recv_all 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid_length); // [確認_異常系] - 最大転送量超過の recv_all 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// 単一ソケット待機の引数不正、タイムアウト、準備完了、失敗が処理されることの確認
TEST_F(socketTest, wait_single_reports_results)
{
    // Arrange
    int ready = 9;
    int ready_after_not_ready = 9;
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の待機 API が 1 個のソケットとタイムアウト 0 を指定して呼び出されること。
    // [Pre-Assert手順] - 下位の待機 API から、タイムアウト、条件成立、失敗の順に応答する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_poll_, poll(_, _, _, _, 1, 0))
        .WillOnce(Return(0))
        .WillOnce(
            [](const char *, const int, const char *, struct pollfd *fds, nfds_t, int)
            {
                fds->revents = 0;
                return 1;
            })
        .WillOnce(
            [](const char *, const int, const char *, struct pollfd *fds, nfds_t, int)
            {
                fds->revents = fds->events;
                return 1;
            })
        .WillOnce(DoAll(Assign(&errno, EBADF), Return(-1)));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, WSAPoll(_, _, _, _, 1, 0))
        .WillOnce(Return(0))
        .WillOnce(
            [](const char *, const int, const char *, LPWSAPOLLFD fds, ULONG, INT)
            {
                fds->revents = 0;
                return 1;
            })
        .WillOnce(
            [](const char *, const int, const char *, LPWSAPOLLFD fds, ULONG, INT)
            {
                fds->revents = POLLRDNORM;
                return 1;
            })
        .WillOnce(Return(SOCKET_ERROR));
    EXPECT_CALL(mock_winsock_, WSAGetLastError).WillOnce(Return(WSAEBADF));
#endif /* PLATFORM_ */

    // Act
    int rtc_invalid_socket = com_util_socket_wait_readable(COM_UTIL_INVALID_SOCKET, 0, &ready,
                                                           &detail); // [手順] - 無効なソケットで待機する。
    int rtc_invalid_output =
        com_util_socket_wait_readable(kSocket, 0, NULL, &detail); // [手順] - 待機結果の出力先に NULL を指定する。
    int rtc_timeout = com_util_socket_wait_readable(kSocket, 0, &ready, &detail); // [手順] - タイムアウトを注入する。
    int rtc_not_ready =
        com_util_socket_wait_readable(kSocket, 0, &ready, &detail); // [手順] - イベント不一致を注入する。
    ready_after_not_ready = ready;
    int rtc_ready = com_util_socket_wait_writable(kSocket, 0, &ready, &detail); // [手順] - 書き込み可能を注入する。
    int rtc_failure =
        com_util_socket_wait_readable(kSocket, 0, &ready, &detail); // [手順] - 待機 API の失敗を注入する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid_socket); // [確認_異常系] - 無効なソケットの待機戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid_output); // [確認_異常系] - NULL 出力先の待機戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_timeout); // [確認_正常系] - 読み取り待機のタイムアウト戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_not_ready); // [確認_正常系] - イベント不一致時の待機戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(0,
              ready_after_not_ready); // [確認_正常系] - イベント不一致時の準備完了フラグが 0 であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_ready); // [確認_正常系] - 書き込み待機の準備完了戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_failure); // [確認_異常系] - 待機 API 失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(0,
              ready); // [確認_正常系] - 最後の待機失敗で準備完了フラグが 0 であること。
}

// 複数ソケット待機の入力検証と全無効ソケット処理が行われることの確認
TEST_F(socketTest, wait_multi_rejects_invalid_and_waits_without_valid_socket)
{
    // Arrange
    const com_util_socket socks[1] = {COM_UTIL_INVALID_SOCKET};
    unsigned char ready[1] = {9U};
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 有効なソケットがない場合に com_util_sleep_ms がタイムアウト時間を引数として 1 回呼び出されること。
    EXPECT_CALL(mock_com_util_, com_util_sleep_ms(5)).Times(1);

    // Act
    int rtc_null_socks =
        com_util_socket_wait_readable_multi(NULL, 1U, 0, ready, &detail); // [手順] - ソケット配列に NULL を指定する。
    int rtc_zero_count =
        com_util_socket_wait_readable_multi(socks, 0U, 0, ready, &detail); // [手順] - 要素数 0 を指定する。
    int rtc_large_count = com_util_socket_wait_readable_multi(socks, COM_UTIL_SOCKET_WAIT_MAX + 1U, 0, ready,
                                                              &detail); // [手順] - 最大数を超える要素数を指定する。
    int rtc_null_ready =
        com_util_socket_wait_readable_multi(socks, 1U, 0, NULL, &detail); // [手順] - 結果配列に NULL を指定する。
    int rtc_wait =
        com_util_socket_wait_readable_multi(socks, 1U, 5, ready, &detail); // [手順] - 全無効ソケットで待機する。
    int rtc_wait_without_timeout = com_util_socket_wait_readable_multi(
        socks, 1U, 0, ready, &detail); // [手順] - 全無効ソケットで待機時間 0 を指定する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              rtc_null_socks); // [確認_異常系] - NULL 配列の待機戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              rtc_zero_count); // [確認_異常系] - 要素数 0 の待機戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              rtc_large_count); // [確認_異常系] - 最大数超過の待機戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              rtc_null_ready); // [確認_異常系] - NULL 結果配列の待機戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_wait); // [確認_正常系] - 全無効ソケットの待機戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(
        COM_UTIL_OK,
        rtc_wait_without_timeout); // [確認_正常系] - 全無効ソケットで待機時間 0 を指定した戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(0U,
              ready[0]); // [確認_正常系] - 全無効ソケットの準備完了フラグが 0 であること。
}

// 複数ソケット待機のタイムアウト、準備完了、失敗が処理されることの確認
TEST_F(socketTest, wait_multi_reports_results)
{
    // Arrange
    const com_util_socket socks[3] = {kSocket, COM_UTIL_INVALID_SOCKET, (com_util_socket)8};
    unsigned char ready[3] = {9U, 9U, 9U};
    unsigned char ready_after_ready[3] = {0U, 0U, 0U};
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の待機 API が 2 個のソケットとタイムアウト 0 を指定して呼び出されること。
    // [Pre-Assert手順] - 下位の待機 API から、一部のソケットが受信可能、いずれも非受信、失敗の順に応答する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_poll_, poll(_, _, _, _, 2, 0))
        .WillOnce(
            [](const char *, const int, const char *, struct pollfd *fds, nfds_t, int)
            {
                fds[0].revents = 0;
                fds[1].revents = POLLERR;
                return 0;
            })
        .WillOnce(
            [](const char *, const int, const char *, struct pollfd *fds, nfds_t, int)
            {
                fds[0].revents = POLLIN;
                fds[1].revents = POLLHUP;
                return 2;
            })
        .WillOnce(DoAll(Assign(&errno, EBADF), Return(-1)));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, WSAPoll(_, _, _, _, 2, 0))
        .WillOnce(
            [](const char *, const int, const char *, LPWSAPOLLFD fds, ULONG, INT)
            {
                fds[0].revents = 0;
                fds[1].revents = POLLERR;
                return 0;
            })
        .WillOnce(
            [](const char *, const int, const char *, LPWSAPOLLFD fds, ULONG, INT)
            {
                fds[0].revents = POLLRDNORM;
                fds[1].revents = POLLHUP;
                return 2;
            })
        .WillOnce(Return(SOCKET_ERROR));
    EXPECT_CALL(mock_winsock_, WSAGetLastError).WillOnce(Return(WSAENETDOWN));
#endif /* PLATFORM_ */

    // Act
    int rtc_timeout =
        com_util_socket_wait_readable_multi(socks, 3U, 0, ready, &detail); // [手順] - タイムアウトを注入する。
    int rtc_ready = com_util_socket_wait_readable_multi(socks, 3U, 0, ready,
                                                        &detail); // [手順] - 複数ソケットの準備完了を注入する。
    std::memcpy(ready_after_ready, ready, sizeof(ready_after_ready));
    int rtc_failure =
        com_util_socket_wait_readable_multi(socks, 3U, 0, ready, &detail); // [手順] - 待機失敗を注入する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              rtc_timeout); // [確認_正常系] - タイムアウト時の複数待機戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_ready); // [確認_正常系] - 準備完了時の複数待機戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(1U,
              ready_after_ready[0]); // [確認_正常系] - 1 番目のソケットが準備完了になること。
    EXPECT_EQ(0U,
              ready_after_ready[1]); // [確認_正常系] - 無効ソケットの準備完了が 0 であること。
    EXPECT_EQ(1U,
              ready_after_ready[2]); // [確認_正常系] - 3 番目のソケットが準備完了になること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_failure); // [確認_異常系] - 待機失敗時の複数待機戻り値が COM_UTIL_ERR_UNKNOWN であること。
}

#if defined(PLATFORM_LINUX)

// 単一ソケット待機がシグナル中断後に再待機することの確認
TEST_F(socketTest, wait_single_retries_after_interrupt)
{
    // Arrange
    int ready = 9;
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の待機 API が 1 個のソケットとタイムアウト 0 を指定して 2 回呼び出されること。
    // [Pre-Assert手順] - 下位の待機 API から、シグナルによる中断ののち条件成立を返却する。
    EXPECT_CALL(mock_poll_, poll(_, _, _, _, 1, 0))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return(-1)))
        .WillOnce(
            [](const char *, const int, const char *, struct pollfd *fds, nfds_t, int)
            {
                fds->revents = fds->events;
                return 1;
            });

    // Act
    int actual_ret = com_util_socket_wait_readable(kSocket, 0, &ready, &detail); // [手順] - タイムアウト 0 で受信可能を待機する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              actual_ret); // [確認_正常系] - com_util_socket_wait_readable の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(1,
              ready); // [確認_正常系] - 再待機で条件が成立し、準備完了フラグが 1 になること。
}

// 無期限の単一ソケット待機がシグナル中断後に期限を計算せず再待機することの確認
TEST_F(socketTest, wait_single_retries_without_deadline_after_interrupt)
{
    // Arrange
    int ready = 9;
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 無期限待機では単調時刻が取得されないこと。
    EXPECT_CALL(mock_com_util_, com_util_get_monotonic_ms()).Times(0);
    // [Pre-Assert確認_正常系] - 下位の待機 API が無期限のタイムアウトを指定して 2 回呼び出されること。
    // [Pre-Assert手順] - 下位の待機 API から、シグナルによる中断ののち条件成立を返却する。
    EXPECT_CALL(mock_poll_, poll(_, _, _, _, 1, COM_UTIL_SOCKET_WAIT_FOREVER))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return(-1)))
        .WillOnce(
            [](const char *, const int, const char *, struct pollfd *fds, nfds_t, int)
            {
                fds->revents = fds->events;
                return 1;
            });

    // Act
    int actual_ret = com_util_socket_wait_readable(kSocket, COM_UTIL_SOCKET_WAIT_FOREVER, &ready,
                                            &detail); // [手順] - 無期限で受信可能を待機する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              actual_ret); // [確認_正常系] - 無期限指定の com_util_socket_wait_readable の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(1,
              ready); // [確認_正常系] - 再待機で条件が成立し、準備完了フラグが 1 になること。
}

// 期限付きの単一ソケット待機がシグナル中断後に残り時間で再待機することの確認
TEST_F(socketTest, wait_single_recomputes_remaining_after_interrupt)
{
    // Arrange
    int ready = 9;
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 単調時刻が待機の開始時と中断時の 2 回取得されること。
    // [Pre-Assert手順] - 単調時刻から、開始時に 1000 ms、中断時に 1040 ms を返却する。
    EXPECT_CALL(mock_com_util_, com_util_get_monotonic_ms())
        .WillOnce(Return((uint64_t)1000U))
        .WillOnce(Return((uint64_t)1040U));
    // [Pre-Assert確認_正常系] - 下位の待機 API が 1 回目に要求どおりの 100 ms を指定して呼び出されること。
    // [Pre-Assert手順] - 下位の待機 API からシグナルによる中断を返却する。
    EXPECT_CALL(mock_poll_, poll(_, _, _, _, 1, 100)).WillOnce(DoAll(Assign(&errno, EINTR), Return(-1)));
    // [Pre-Assert確認_正常系] - 下位の待機 API が 2 回目に残り時間の 60 ms を指定して呼び出されること。
    // [Pre-Assert手順] - 下位の待機 API から条件成立を返却する。
    EXPECT_CALL(mock_poll_, poll(_, _, _, _, 1, 60))
        .WillOnce(
            [](const char *, const int, const char *, struct pollfd *fds, nfds_t, int)
            {
                fds->revents = fds->events;
                return 1;
            });

    // Act
    int actual_ret =
        com_util_socket_wait_readable(kSocket, 100, &ready, &detail); // [手順] - タイムアウト 100 ms で待機する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              actual_ret); // [確認_正常系] - タイムアウト 100 ms の com_util_socket_wait_readable の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(1,
              ready); // [確認_正常系] - 残り時間での再待機で条件が成立し、準備完了フラグが 1 になること。
}

// 期限付きの単一ソケット待機がシグナル中断で期限を過ぎた場合に条件不成立となることの確認
TEST_F(socketTest, wait_single_reports_not_ready_when_deadline_expires_after_interrupt)
{
    // Arrange
    int ready = 9;
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 単調時刻が待機の開始時と中断時の 2 回取得されること。
    // [Pre-Assert手順] - 単調時刻から、開始時に 1000 ms、中断時に期限を過ぎた 1100 ms を返却する。
    EXPECT_CALL(mock_com_util_, com_util_get_monotonic_ms())
        .WillOnce(Return((uint64_t)1000U))
        .WillOnce(Return((uint64_t)1100U));
    // [Pre-Assert確認_正常系] - 下位の待機 API が 50 ms を指定して 1 回だけ呼び出されること。
    // [Pre-Assert手順] - 下位の待機 API からシグナルによる中断を返却する。
    EXPECT_CALL(mock_poll_, poll(_, _, _, _, 1, 50)).WillOnce(DoAll(Assign(&errno, EINTR), Return(-1)));

    // Act
    int actual_ret = com_util_socket_wait_readable(kSocket, 50, &ready, &detail); // [手順] - タイムアウト 50 ms で待機する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        actual_ret); // [確認_正常系] - 期限超過後の com_util_socket_wait_readable の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(0,
              ready); // [確認_正常系] - 期限を過ぎたため準備完了フラグが 0 のままであること。
}

// 複数ソケット待機がシグナル中断後に再待機することの確認
TEST_F(socketTest, wait_multi_retries_after_interrupt)
{
    // Arrange
    const com_util_socket socks[2] = {kSocket, (com_util_socket)8};
    unsigned char ready[2] = {9U, 9U};
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の待機 API が 2 個のソケットとタイムアウト 0 を指定して 2 回呼び出されること。
    // [Pre-Assert手順] - 下位の待機 API から、シグナルによる中断ののち 1 番目のソケットの受信可能を返却する。
    EXPECT_CALL(mock_poll_, poll(_, _, _, _, 2, 0))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return(-1)))
        .WillOnce(
            [](const char *, const int, const char *, struct pollfd *fds, nfds_t, int)
            {
                fds[0].revents = POLLIN;
                fds[1].revents = 0;
                return 1;
            });

    // Act
    int actual_ret = com_util_socket_wait_readable_multi(socks, 2U, 0, ready,
                                                  &detail); // [手順] - 2 個のソケットで受信可能を待機する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              actual_ret); // [確認_正常系] - com_util_socket_wait_readable_multi の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(1U,
              ready[0]); // [確認_正常系] - 再待機で 1 番目のソケットが準備完了になること。
    EXPECT_EQ(0U,
              ready[1]); // [確認_正常系] - 2 番目のソケットの準備完了が 0 であること。
}

// 接続受け付けがシグナル中断後に再試行することの確認
TEST_F(socketTest, accept_retries_after_interrupt)
{
    // Arrange
    com_util_socket accepted = COM_UTIL_INVALID_SOCKET;
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の accept API が待ち受けソケットを引数として 2 回呼び出されること。
    // [Pre-Assert手順] - 下位の accept API から、シグナルによる中断ののち新しいソケットのハンドルを返却する。
    EXPECT_CALL(mock_sys_socket_, accept(_, _, _, (int)kSocket, _, _))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return(-1)))
        .WillOnce(Return(8));

    // Act
    int actual_ret = com_util_socket_accept(kSocket, NULL, &accepted, &detail); // [手順] - 接続を受け付ける。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              actual_ret); // [確認_正常系] - com_util_socket_accept の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ((com_util_socket)8,
              accepted); // [確認_正常系] - 再試行で受け付けたソケットが返されること。
}

// 送信と受信がシグナル中断後に再試行することの確認
TEST_F(socketTest, send_and_recv_retry_after_interrupt)
{
    // Arrange
    unsigned char buffer[4] = {0U, 0U, 0U, 0U};
    size_t sent = 0U;
    size_t received = 0U;
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の send API が MSG_NOSIGNAL と 4 バイトを指定して 2 回呼び出されること。
    // [Pre-Assert手順] - 下位の send API から、シグナルによる中断ののち 4 バイトの転送を返却する。
    EXPECT_CALL(mock_sys_socket_, send(_, _, _, (int)kSocket, _, 4U, MSG_NOSIGNAL))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return((ssize_t)-1)))
        .WillOnce(Return((ssize_t)4));
    // [Pre-Assert確認_正常系] - 下位の recv API が 4 バイトを指定して 2 回呼び出されること。
    // [Pre-Assert手順] - 下位の recv API から、シグナルによる中断ののち 4 バイトの転送を返却する。
    EXPECT_CALL(mock_sys_socket_, recv(_, _, _, (int)kSocket, _, 4U, 0))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return((ssize_t)-1)))
        .WillOnce(Return((ssize_t)4));

    // Act
    int rtc_send = com_util_socket_send(kSocket, buffer, sizeof(buffer), &sent, &detail); // [手順] - 4 バイト送信する。
    int rtc_recv = com_util_socket_recv(kSocket, buffer, sizeof(buffer), &received, &detail); // [手順] - 4 バイト受信する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              rtc_send); // [確認_正常系] - com_util_socket_send の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ((size_t)4,
              sent); // [確認_正常系] - 再試行後の送信バイト数が 4 であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_recv); // [確認_正常系] - com_util_socket_recv の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ((size_t)4,
              received); // [確認_正常系] - 再試行後の受信バイト数が 4 であること。
}

// データグラムの送信と受信がシグナル中断後に再試行することの確認
TEST_F(socketTest, sendto_and_recvfrom_retry_after_interrupt)
{
    // Arrange
    unsigned char buffer[4] = {0U, 0U, 0U, 0U};
    com_util_ipv4_endpoint peer = {};
    size_t sent = 0U;
    size_t received = 0U;
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の sendto API が 4 バイトを指定して 2 回呼び出されること。
    // [Pre-Assert手順] - 下位の sendto API から、シグナルによる中断ののち 4 バイトの転送を返却する。
    EXPECT_CALL(mock_sys_socket_, sendto(_, _, _, (int)kSocket, _, 4U, 0, _, _))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return((ssize_t)-1)))
        .WillOnce(Return((ssize_t)4));
    // [Pre-Assert確認_正常系] - 下位の recvfrom API が 4 バイトを指定して 2 回呼び出されること。
    // [Pre-Assert手順] - 下位の recvfrom API から、シグナルによる中断ののち 4 バイトの転送を返却する。
    EXPECT_CALL(mock_sys_socket_, recvfrom(_, _, _, (int)kSocket, _, 4U, 0, _, _))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return((ssize_t)-1)))
        .WillOnce(Return((ssize_t)4));

    // Act
    int rtc_sendto = com_util_socket_sendto(kSocket, buffer, sizeof(buffer), &kEndpoint, &sent,
                                            &detail); // [手順] - 4 バイトを指定した端点へ送信する。
    int rtc_recvfrom = com_util_socket_recvfrom(kSocket, buffer, sizeof(buffer), &peer, &received,
                                                &detail); // [手順] - 4 バイトを受信する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              rtc_sendto); // [確認_正常系] - com_util_socket_sendto の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ((size_t)4,
              sent); // [確認_正常系] - 再試行後の送信バイト数が 4 であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_recvfrom); // [確認_正常系] - com_util_socket_recvfrom の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ((size_t)4,
              received); // [確認_正常系] - 再試行後の受信バイト数が 4 であること。
}

// 全量送信と全量受信がシグナル中断後に再試行することの確認
TEST_F(socketTest, send_all_and_recv_all_retry_after_interrupt)
{
    // Arrange
    unsigned char buffer[4] = {0U, 0U, 0U, 0U};
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の send API が MSG_NOSIGNAL と 4 バイトを指定して 2 回呼び出されること。
    // [Pre-Assert手順] - 下位の send API から、シグナルによる中断ののち 4 バイトの転送を返却する。
    EXPECT_CALL(mock_sys_socket_, send(_, _, _, (int)kSocket, _, 4U, MSG_NOSIGNAL))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return((ssize_t)-1)))
        .WillOnce(Return((ssize_t)4));
    // [Pre-Assert確認_正常系] - 下位の recv API が 4 バイトを指定して 2 回呼び出されること。
    // [Pre-Assert手順] - 下位の recv API から、シグナルによる中断ののち 4 バイトの転送を返却する。
    EXPECT_CALL(mock_sys_socket_, recv(_, _, _, (int)kSocket, _, 4U, 0))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return((ssize_t)-1)))
        .WillOnce(Return((ssize_t)4));

    // Act
    int rtc_send_all =
        com_util_socket_send_all(kSocket, buffer, sizeof(buffer), &detail); // [手順] - 4 バイトを全量送信する。
    int rtc_recv_all =
        com_util_socket_recv_all(kSocket, buffer, sizeof(buffer), &detail); // [手順] - 4 バイトを全量受信する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              rtc_send_all); // [確認_正常系] - com_util_socket_send_all の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_recv_all); // [確認_正常系] - com_util_socket_recv_all の戻り値が COM_UTIL_OK であること。
}

// ブロッキング接続がシグナル中断後に完了を待って成功を確定することの確認
TEST_F(socketTest, connect_completes_after_interrupt)
{
    // Arrange
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の connect API が 1 回だけ呼び出されること。
    // [Pre-Assert手順] - 下位の connect API からシグナルによる中断を返却する。
    EXPECT_CALL(mock_sys_socket_, connect(_, _, _, (int)kSocket, _, _))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return(-1)));
    // [Pre-Assert確認_正常系] - 下位の待機 API が無期限のタイムアウトを指定して呼び出されること。
    // [Pre-Assert手順] - 下位の待機 API から書き込み可能を返却する。
    EXPECT_CALL(mock_poll_, poll(_, _, _, _, 1, COM_UTIL_SOCKET_WAIT_FOREVER))
        .WillOnce(
            [](const char *, const int, const char *, struct pollfd *fds, nfds_t, int)
            {
                fds->revents = fds->events;
                return 1;
            });
    // [Pre-Assert確認_正常系] - 下位の getsockopt API が SOL_SOCKET と SO_ERROR を指定して呼び出されること。
    // [Pre-Assert手順] - 下位の getsockopt API から保留エラーなしを返却する。
    EXPECT_CALL(mock_sys_socket_, getsockopt(_, _, _, (int)kSocket, SOL_SOCKET, SO_ERROR, _, _))
        .WillOnce(
            [](const char *, const int, const char *, int, int, int, void *value, socklen_t *)
            {
                *static_cast<int *>(value) = 0;
                return 0;
            });

    // Act
    int actual_ret = com_util_socket_connect(kSocket, &kEndpoint, &detail); // [手順] - ブロッキング モードで接続する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              actual_ret); // [確認_正常系] - 中断後の com_util_socket_connect の戻り値が COM_UTIL_OK であること。
}

// ブロッキング接続がシグナル中断後の完了確認で保留エラーを検出することの確認
TEST_F(socketTest, connect_reports_pending_error_after_interrupt)
{
    // Arrange
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_異常系] - 下位の connect API が 1 回だけ呼び出されること。
    // [Pre-Assert手順] - 下位の connect API からシグナルによる中断を返却する。
    EXPECT_CALL(mock_sys_socket_, connect(_, _, _, (int)kSocket, _, _))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return(-1)));
    // [Pre-Assert確認_異常系] - 下位の待機 API が無期限のタイムアウトを指定して呼び出されること。
    // [Pre-Assert手順] - 下位の待機 API から書き込み可能を返却する。
    EXPECT_CALL(mock_poll_, poll(_, _, _, _, 1, COM_UTIL_SOCKET_WAIT_FOREVER))
        .WillOnce(
            [](const char *, const int, const char *, struct pollfd *fds, nfds_t, int)
            {
                fds->revents = fds->events;
                return 1;
            });
    // [Pre-Assert確認_異常系] - 下位の getsockopt API が SOL_SOCKET と SO_ERROR を指定して呼び出されること。
    // [Pre-Assert手順] - 下位の getsockopt API から接続拒否の保留エラーを返却する。
    EXPECT_CALL(mock_sys_socket_, getsockopt(_, _, _, (int)kSocket, SOL_SOCKET, SO_ERROR, _, _))
        .WillOnce(
            [](const char *, const int, const char *, int, int, int, void *value, socklen_t *)
            {
                *static_cast<int *>(value) = ECONNREFUSED;
                return 0;
            });

    // Act
    int actual_ret = com_util_socket_connect(kSocket, &kEndpoint, &detail); // [手順] - ブロッキング モードで接続する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              actual_ret); // [確認_異常系] - 保留エラーがある場合の com_util_socket_connect の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    expect_detail(detail, COM_UTIL_ERROR_DOMAIN_SOCKET_ERRNO, COM_UTIL_ERR_UNKNOWN,
                  (unsigned long)ECONNREFUSED); // [確認_異常系] - 詳細に接続拒否が記録されること。
}

// ブロッキング接続がシグナル中断後の待機失敗を通知することの確認
TEST_F(socketTest, connect_reports_wait_failure_after_interrupt)
{
    // Arrange
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_異常系] - 下位の connect API が 1 回だけ呼び出されること。
    // [Pre-Assert手順] - 下位の connect API からシグナルによる中断を返却する。
    EXPECT_CALL(mock_sys_socket_, connect(_, _, _, (int)kSocket, _, _))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return(-1)));
    // [Pre-Assert確認_異常系] - 下位の待機 API が無期限のタイムアウトを指定して呼び出されること。
    // [Pre-Assert手順] - 下位の待機 API から不正な記述子による失敗を返却する。
    EXPECT_CALL(mock_poll_, poll(_, _, _, _, 1, COM_UTIL_SOCKET_WAIT_FOREVER))
        .WillOnce(DoAll(Assign(&errno, EBADF), Return(-1)));
    // [Pre-Assert確認_異常系] - 下位の getsockopt API が呼び出されないこと。
    EXPECT_CALL(mock_sys_socket_, getsockopt(_, _, _, _, _, _, _, _)).Times(0);

    // Act
    int actual_ret = com_util_socket_connect(kSocket, &kEndpoint, &detail); // [手順] - ブロッキング モードで接続する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        actual_ret); // [確認_異常系] - 待機に失敗した場合の com_util_socket_connect の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    expect_detail(detail, COM_UTIL_ERROR_DOMAIN_SOCKET_ERRNO, COM_UTIL_ERR_UNKNOWN,
                  (unsigned long)EBADF); // [確認_異常系] - 詳細に待機失敗の要因が記録されること。
}

// ブロッキング接続がシグナル中断後の待機で条件不成立となった場合にタイムアウトを通知することの確認
TEST_F(socketTest, connect_reports_timeout_when_not_writable_after_interrupt)
{
    // Arrange
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_異常系] - 下位の connect API が 1 回だけ呼び出されること。
    // [Pre-Assert手順] - 下位の connect API からシグナルによる中断を返却する。
    EXPECT_CALL(mock_sys_socket_, connect(_, _, _, (int)kSocket, _, _))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return(-1)));
    // [Pre-Assert確認_異常系] - 下位の待機 API が無期限のタイムアウトを指定して呼び出されること。
    // [Pre-Assert手順] - 下位の待機 API から条件不成立を返却する。
    EXPECT_CALL(mock_poll_, poll(_, _, _, _, 1, COM_UTIL_SOCKET_WAIT_FOREVER)).WillOnce(Return(0));
    // [Pre-Assert確認_異常系] - 下位の getsockopt API が呼び出されないこと。
    EXPECT_CALL(mock_sys_socket_, getsockopt(_, _, _, _, _, _, _, _)).Times(0);

    // Act
    int actual_ret = com_util_socket_connect(kSocket, &kEndpoint, &detail); // [手順] - ブロッキング モードで接続する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_TIMEOUT,
        actual_ret); // [確認_異常系] - 条件不成立の場合の com_util_socket_connect の戻り値が COM_UTIL_ERR_TIMEOUT であること。
}

#endif /* PLATFORM_LINUX */

// 受信停止の引数不正、成功、失敗が処理されることの確認
TEST_F(socketTest, shutdown_receive_reports_results)
{
    // Arrange
    com_util_socket socket = kSocket;
    com_util_socket failure_socket = kSocket;
    com_util_socket invalid_socket = COM_UTIL_INVALID_SOCKET;
    com_util_error detail = {};

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の受信停止 API が対象のソケットを引数として 2 回呼び出されること。
    // [Pre-Assert手順] - 下位の受信停止 API から、成功ののち失敗を返却し、失敗要因を通知する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_socket_, shutdown(_, _, _, (int)kSocket, SHUT_RD))
        .WillOnce(Return(0))
        .WillOnce(DoAll(Assign(&errno, EBADF), Return(-1)));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, closesocket(_, _, _, (SOCKET)kSocket))
        .WillOnce(Return(0))
        .WillOnce(Return(SOCKET_ERROR));
    EXPECT_CALL(mock_winsock_, WSAGetLastError).WillOnce(Return(WSAEBADF));
#endif /* PLATFORM_ */

    // Act
    int rtc_null = com_util_socket_shutdown_receive(NULL, &detail); // [手順] - ソケット出力先に NULL を指定する。
    int rtc_invalid = com_util_socket_shutdown_receive(&invalid_socket, &detail); // [手順] - 無効なソケットを指定する。
    int rtc_success = com_util_socket_shutdown_receive(&socket, &detail);         // [手順] - 受信停止を成功させる。
    int rtc_failure = com_util_socket_shutdown_receive(&failure_socket, &detail); // [手順] - 受信停止の失敗を注入する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              rtc_null); // [確認_異常系] - NULL の出力先を指定した戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_invalid); // [確認_異常系] - 無効なソケットを指定した戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_success); // [確認_正常系] - 受信停止成功時の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_failure); // [確認_異常系] - 受信停止失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
#if defined(PLATFORM_WINDOWS)
    EXPECT_EQ(COM_UTIL_INVALID_SOCKET,
              socket); // [確認_正常系] - Windows の受信停止成功でソケットが無効値になること。
#endif                 /* PLATFORM_WINDOWS */
}
