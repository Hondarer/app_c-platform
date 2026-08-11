#include <testfw.h>

#include <com_util/base/platform.h>
#include <com_util/base/result.h>
#include <com_util/net/endpoint.h>
#include <errno.h>

#if defined(PLATFORM_LINUX)
    #include <arpa/mock_inet.h>
    #include <mock_netdb.h>

    #include <arpa/inet.h>
    #include <netdb.h>
    #include <netinet/in.h>
#elif defined(PLATFORM_WINDOWS)
    #include <com_util/net/socket_internal.h>
    #include <mock_com_util.h>
    #include <mock_winsock.h>
#endif /* PLATFORM_ */

#include <cstring>

using testing::_;
using testing::Assign;
using testing::DoAll;
using testing::NiceMock;
using testing::Return;

namespace
{

void expect_detail(const com_util_error &detail, const com_util_error_domain domain, const int result,
                   const unsigned long code)
{
    EXPECT_EQ(domain, detail.domain);
    EXPECT_EQ(result, detail.result);
    EXPECT_EQ(code, detail.code);
}

} // namespace

class endpointTest : public Test
{
  protected:
#if defined(PLATFORM_WINDOWS)
    NiceMock<Mock_com_util> mock_com_util_;
    NiceMock<Mock_winsock> mock_winsock_;

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

// IPv4 の解析が NULL 引数を拒否することの確認
TEST_F(endpointTest, parse_rejects_null_arguments)
{
    // Arrange
    uint32_t address = 0U;

    // Pre-Assert

    // Act
    int rtc_null_text = com_util_ipv4_parse(NULL, &address);      // [手順] - text に NULL を指定して解析する。
    int rtc_null_output = com_util_ipv4_parse("192.0.2.1", NULL); // [手順] - address_out に NULL を指定して解析する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_null_text); // [確認_異常系] - text が NULL の com_util_ipv4_parse の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_null_output); // [確認_異常系] - address_out が NULL の com_util_ipv4_parse の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// 不正な IPv4 文字列が拒否されることの確認
TEST_F(endpointTest, parse_rejects_malformed_text)
{
    // Arrange
    uint32_t address = 0xA5A5A5A5U;

#if defined(PLATFORM_LINUX)
    NiceMock<Mock_arpa_inet> mock_arpa_inet;

    // Pre-Assert
    EXPECT_CALL(mock_arpa_inet, inet_pton(_, _, _, _, _, _))
        .WillOnce(Return(0)); // [Pre-Assert確認_異常系] - inet_pton が不正な形式を示す 0 を返すこと。
#elif defined(PLATFORM_WINDOWS)
    // Pre-Assert
    EXPECT_CALL(mock_winsock_, inet_pton(_, _, _, _, _, _))
        .WillOnce(Return(0)); // [Pre-Assert確認_異常系] - inet_pton が不正な形式を示す 0 を返すこと。
#endif /* PLATFORM_ */

    // Act
    int rtc = com_util_ipv4_parse("not-an-ip", &address); // [手順] - 不正な IPv4 文字列を解析する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc); // [確認_異常系] - 不正な文字列を指定した com_util_ipv4_parse の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(0xA5A5A5A5U,
              address); // [確認_異常系] - 解析に失敗して address_out が変更されないこと。
}

// 正しい IPv4 文字列がネットワークバイトオーダーの値へ変換されることの確認
TEST_F(endpointTest, parse_converts_valid_text)
{
    // Arrange
    uint32_t address = 0U;
    const uint32_t expected = COM_UTIL_IPV4_ADDR_LOOPBACK;

#if defined(PLATFORM_LINUX)
    NiceMock<Mock_arpa_inet> mock_arpa_inet;

    // Pre-Assert
    EXPECT_CALL(mock_arpa_inet, inet_pton(_, _, _, _, _, _))
        .WillOnce(
            [expected](const char *, const int, const char *, int, const char *, void *dst)
            {
                static_cast<struct in_addr *>(dst)->s_addr = expected;
                return 1;
            }); // [Pre-Assert確認_正常系] - inet_pton がループバックアドレスの変換に 1 を返すこと。
#elif defined(PLATFORM_WINDOWS)
    // Pre-Assert
    EXPECT_CALL(mock_winsock_, inet_pton(_, _, _, _, _, _))
        .WillOnce(
            [expected](const char *, const int, const char *, INT, PCSTR, PVOID dst)
            {
                static_cast<IN_ADDR *>(dst)->S_un.S_addr = expected;
                return 1;
            }); // [Pre-Assert確認_正常系] - inet_pton がループバックアドレスの変換に 1 を返すこと。
#endif /* PLATFORM_ */

    // Act
    int rtc = com_util_ipv4_parse("127.0.0.1", &address); // [手順] - ループバックアドレスを解析する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        rtc); // [確認_正常系] - 正しい IPv4 文字列を指定した com_util_ipv4_parse の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(expected,
              address); // [確認_正常系] - com_util_ipv4_parse がネットワークバイトオーダーのアドレスを返すこと。
}

#if defined(PLATFORM_WINDOWS)
// Winsock の初期化に失敗した場合、IPv4 解析が不正引数として終了することの確認
TEST_F(endpointTest, parse_returns_invalid_when_startup_fails)
{
    // Arrange
    uint32_t address = 0U;

    // Pre-Assert
    EXPECT_CALL(mock_winsock_, WSAStartup(_, _, _, _, _))
        .WillOnce(Return(WSASYSNOTREADY)); // [Pre-Assert確認_異常系] - WSAStartup が初期化失敗を返すこと。

    // Act
    int rtc = com_util_ipv4_parse("127.0.0.1", &address); // [手順] - 初期化失敗を注入して IPv4 を解析する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc); // [確認_異常系] - 初期化に失敗した com_util_ipv4_parse の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}
#endif /* PLATFORM_WINDOWS */

// 名前解決が NULL 引数を拒否することの確認
TEST_F(endpointTest, resolve_rejects_null_arguments)
{
    // Arrange
    uint32_t address = 0U;
    com_util_error detail = {};

    // Pre-Assert

    // Act
    int rtc_null_text =
        com_util_ipv4_resolve(NULL, &address, &detail); // [手順] - text に NULL を指定して名前解決する。
    int rtc_null_output =
        com_util_ipv4_resolve("localhost", NULL, &detail); // [手順] - address_out に NULL を指定して名前解決する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_null_text); // [確認_異常系] - text が NULL の com_util_ipv4_resolve の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_null_output); // [確認_異常系] - address_out が NULL の com_util_ipv4_resolve の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    expect_detail(detail, COM_UTIL_ERROR_DOMAIN_ERRNO, COM_UTIL_ERR_INVALID_ARGUMENT,
                  static_cast<unsigned long>(EINVAL));
}

// 名前解決の失敗時に GAI エラーが返されることの確認
TEST_F(endpointTest, resolve_reports_lookup_failure)
{
    // Arrange
    uint32_t address = 0U;
    com_util_error detail = {};

#if defined(PLATFORM_LINUX)
    NiceMock<Mock_netdb> mock_netdb;

    // Pre-Assert
    EXPECT_CALL(mock_netdb, getaddrinfo(_, _, _, _, _, _, _))
        .WillOnce(
            [](const char *, const int, const char *, const char *, const char *, const struct addrinfo *,
               struct addrinfo **result)
            {
                *result = NULL;
                return EAI_AGAIN;
            }); // [Pre-Assert確認_異常系] - getaddrinfo が一時的な解決失敗を返すこと。
#elif defined(PLATFORM_WINDOWS)
    // Pre-Assert
    EXPECT_CALL(mock_winsock_, getaddrinfo(_, _, _, _, _, _, _))
        .WillOnce(
            [](const char *, const int, const char *, PCSTR, PCSTR, const ADDRINFOA *, PADDRINFOA *result)
            {
                *result = NULL;
                return EAI_AGAIN;
            }); // [Pre-Assert確認_異常系] - getaddrinfo が一時的な解決失敗を返すこと。
#endif /* PLATFORM_ */

    // Act
    int rtc = com_util_ipv4_resolve("example.invalid", &address, &detail); // [手順] - 名前解決失敗を注入する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        rtc); // [確認_異常系] - 名前解決に失敗した com_util_ipv4_resolve の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    expect_detail(detail, COM_UTIL_ERROR_DOMAIN_GAI, COM_UTIL_ERR_UNKNOWN,
#if defined(PLATFORM_LINUX)
                  static_cast<unsigned long>(EAI_AGAIN));
#else
                  static_cast<unsigned long>(EAI_AGAIN));
#endif /* PLATFORM_ */
}

// 名前解決の失敗時に返された解決結果が解放されることの確認
TEST_F(endpointTest, resolve_releases_result_when_lookup_fails)
{
    // Arrange
    uint32_t address = 0U;
    com_util_error detail = {};

#if defined(PLATFORM_LINUX)
    struct addrinfo resolved = {};
    NiceMock<Mock_netdb> mock_netdb;

    // Pre-Assert
    EXPECT_CALL(mock_netdb, getaddrinfo(_, _, _, _, _, _, _))
        .WillOnce(
            [&resolved](const char *, const int, const char *, const char *, const char *, const struct addrinfo *,
                        struct addrinfo **result)
            {
                *result = &resolved;
                return EAI_FAIL;
            }); // [Pre-Assert確認_異常系] - getaddrinfo が解決結果を残したまま失敗すること。
    EXPECT_CALL(mock_netdb, freeaddrinfo(_, _, _, _))
        .WillOnce(
            [&resolved](const char *, const int, const char *, struct addrinfo *actual)
            {
                EXPECT_EQ(&resolved, actual);
            }); // [Pre-Assert確認_正常系] - 失敗時に残存した解決結果が 1 回解放されること。
#elif defined(PLATFORM_WINDOWS)
    ADDRINFOA resolved = {};

    // Pre-Assert
    EXPECT_CALL(mock_winsock_, getaddrinfo(_, _, _, _, _, _, _))
        .WillOnce(
            [&resolved](const char *, const int, const char *, PCSTR, PCSTR, const ADDRINFOA *, PADDRINFOA *result)
            {
                *result = &resolved;
                return EAI_FAIL;
            }); // [Pre-Assert確認_異常系] - getaddrinfo が解決結果を残したまま失敗すること。
    EXPECT_CALL(mock_winsock_, freeaddrinfo(_, _, _, _))
        .WillOnce(
            [&resolved](const char *, const int, const char *, PADDRINFOA actual)
            {
                EXPECT_EQ(&resolved, actual);
            }); // [Pre-Assert確認_正常系] - 失敗時に残存した解決結果が 1 回解放されること。
#endif /* PLATFORM_ */

    // Act
    int rtc = com_util_ipv4_resolve("example.invalid", &address, &detail); // [手順] - 解決結果を残す失敗を注入する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        rtc); // [確認_異常系] - 解決結果を残した com_util_ipv4_resolve の戻り値が COM_UTIL_ERR_UNKNOWN であること。
}

// 名前解決結果が NULL の場合に失敗として扱われることの確認
TEST_F(endpointTest, resolve_rejects_empty_result)
{
    // Arrange
    uint32_t address = 0U;
    com_util_error detail = {};

#if defined(PLATFORM_LINUX)
    NiceMock<Mock_netdb> mock_netdb;

    // Pre-Assert
    EXPECT_CALL(mock_netdb, getaddrinfo(_, _, _, _, _, _, _))
        .WillOnce(
            [](const char *, const int, const char *, const char *, const char *, const struct addrinfo *,
               struct addrinfo **result)
            {
                *result = NULL;
                return 0;
            }); // [Pre-Assert確認_正常系] - getaddrinfo が成功しながら NULL の結果を返すこと。
#elif defined(PLATFORM_WINDOWS)
    // Pre-Assert
    EXPECT_CALL(mock_winsock_, getaddrinfo(_, _, _, _, _, _, _))
        .WillOnce(
            [](const char *, const int, const char *, PCSTR, PCSTR, const ADDRINFOA *, PADDRINFOA *result)
            {
                *result = NULL;
                return 0;
            }); // [Pre-Assert確認_正常系] - getaddrinfo が成功しながら NULL の結果を返すこと。
#endif /* PLATFORM_ */

    // Act
    int rtc = com_util_ipv4_resolve("localhost", &address, &detail); // [手順] - NULL の名前解決結果を処理する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_NOT_FOUND,
        rtc); // [確認_異常系] - 結果が NULL の com_util_ipv4_resolve の戻り値が COM_UTIL_ERR_NOT_FOUND であること。
}

// 名前解決結果の先頭 IPv4 アドレスが返されることの確認
TEST_F(endpointTest, resolve_returns_first_ipv4_address)
{
    // Arrange
    uint32_t address = 0U;
    com_util_error detail = {};
    const uint32_t expected = COM_UTIL_IPV4_ADDR_LOOPBACK;

#if defined(PLATFORM_LINUX)
    struct sockaddr_in native = {};
    struct addrinfo resolved = {};
    native.sin_addr.s_addr = expected;
    resolved.ai_addr = reinterpret_cast<struct sockaddr *>(&native);
    NiceMock<Mock_netdb> mock_netdb;

    // Pre-Assert
    EXPECT_CALL(mock_netdb, getaddrinfo(_, _, _, _, _, _, _))
        .WillOnce(
            [&resolved](const char *, const int, const char *, const char *, const char *, const struct addrinfo *,
                        struct addrinfo **result)
            {
                *result = &resolved;
                return 0;
            }); // [Pre-Assert確認_正常系] - getaddrinfo が有効な IPv4 解決結果を返すこと。
    EXPECT_CALL(mock_netdb, freeaddrinfo(_, _, _, _))
        .WillOnce(
            [&resolved](const char *, const int, const char *, struct addrinfo *actual)
            { EXPECT_EQ(&resolved, actual); }); // [Pre-Assert確認_正常系] - 成功した解決結果が 1 回解放されること。
#elif defined(PLATFORM_WINDOWS)
    SOCKADDR_IN native = {};
    ADDRINFOA resolved = {};
    native.sin_addr.S_un.S_addr = expected;
    resolved.ai_addr = reinterpret_cast<sockaddr *>(&native);

    // Pre-Assert
    EXPECT_CALL(mock_winsock_, getaddrinfo(_, _, _, _, _, _, _))
        .WillOnce(
            [&resolved](const char *, const int, const char *, PCSTR, PCSTR, const ADDRINFOA *, PADDRINFOA *result)
            {
                *result = &resolved;
                return 0;
            }); // [Pre-Assert確認_正常系] - getaddrinfo が有効な IPv4 解決結果を返すこと。
    EXPECT_CALL(mock_winsock_, freeaddrinfo(_, _, _, _))
        .WillOnce(
            [&resolved](const char *, const int, const char *, PADDRINFOA actual)
            { EXPECT_EQ(&resolved, actual); }); // [Pre-Assert確認_正常系] - 成功した解決結果が 1 回解放されること。
#endif /* PLATFORM_ */

    // Act
    int rtc = com_util_ipv4_resolve("localhost", &address, &detail); // [手順] - 有効な IPv4 解決結果を処理する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              rtc); // [確認_正常系] - 有効な結果を指定した com_util_ipv4_resolve の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(expected,
              address); // [確認_正常系] - com_util_ipv4_resolve が先頭の IPv4 アドレスを返すこと。
    expect_detail(detail, COM_UTIL_ERROR_DOMAIN_NONE, COM_UTIL_OK, 0UL);
}

#if defined(PLATFORM_WINDOWS)
// Winsock の初期化に失敗した場合、名前解決が失敗することの確認
TEST_F(endpointTest, resolve_propagates_startup_failure)
{
    // Arrange
    uint32_t address = 0U;
    com_util_error detail = {};

    // Pre-Assert
    EXPECT_CALL(mock_winsock_, WSAStartup(_, _, _, _, _))
        .WillOnce(Return(WSASYSNOTREADY)); // [Pre-Assert確認_異常系] - WSAStartup が初期化失敗を返すこと。

    // Act
    int rtc = com_util_ipv4_resolve("localhost", &address, &detail); // [手順] - 初期化失敗を注入して名前解決する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        rtc); // [確認_異常系] - 初期化に失敗した com_util_ipv4_resolve の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    expect_detail(detail, COM_UTIL_ERROR_DOMAIN_WINSOCK, COM_UTIL_ERR_UNKNOWN,
                  static_cast<unsigned long>(WSASYSNOTREADY));
}
#endif /* PLATFORM_WINDOWS */

// IPv4 文字列出力が NULL 引数を拒否することの確認
TEST_F(endpointTest, to_string_rejects_null_or_zero_sized_buffer)
{
    // Arrange
    char buffer[COM_UTIL_IPV4_ADDR_STRLEN] = {};
    com_util_error detail = {};

    // Pre-Assert

    // Act
    int rtc_null_buffer = com_util_ipv4_to_string(COM_UTIL_IPV4_ADDR_LOOPBACK, NULL, sizeof(buffer),
                                                  &detail); // [手順] - buffer に NULL を指定して文字列化する。
    int rtc_zero_size = com_util_ipv4_to_string(COM_UTIL_IPV4_ADDR_LOOPBACK, buffer, 0U,
                                                &detail); // [手順] - buffer_size に 0 を指定して文字列化する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_null_buffer); // [確認_異常系] - buffer が NULL の com_util_ipv4_to_string の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_zero_size); // [確認_異常系] - buffer_size が 0 の com_util_ipv4_to_string の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// IPv4 文字列出力が小さいバッファーを拒否することの確認
TEST_F(endpointTest, to_string_rejects_small_buffer)
{
    // Arrange
    char buffer[COM_UTIL_IPV4_ADDR_STRLEN] = {};
    com_util_error detail = {};

    // Pre-Assert

    // Act
    int rtc = com_util_ipv4_to_string(COM_UTIL_IPV4_ADDR_LOOPBACK, buffer, COM_UTIL_IPV4_ADDR_STRLEN - 1U,
                                      &detail); // [手順] - 必要長未満のバッファーで文字列化する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_BUFFER_TOO_SMALL,
        rtc); // [確認_異常系] - 小さいバッファーを指定した com_util_ipv4_to_string の戻り値が COM_UTIL_ERR_BUFFER_TOO_SMALL であること。
    expect_detail(detail, COM_UTIL_ERROR_DOMAIN_ERRNO, COM_UTIL_ERR_BUFFER_TOO_SMALL,
                  static_cast<unsigned long>(ERANGE));
}

// OS の IPv4 文字列化に失敗した場合にエラーが返されることの確認
TEST_F(endpointTest, to_string_reports_conversion_failure)
{
    // Arrange
    char buffer[COM_UTIL_IPV4_ADDR_STRLEN] = {};
    com_util_error detail = {};

#if defined(PLATFORM_LINUX)
    NiceMock<Mock_arpa_inet> mock_arpa_inet;

    // Pre-Assert
    EXPECT_CALL(mock_arpa_inet, inet_ntop(_, _, _, _, _, _, _))
        .WillOnce(DoAll(Assign(&errno, EINVAL), Return(static_cast<const char *>(NULL))));
    // [Pre-Assert確認_異常系] - inet_ntop が NULL を返すこと。
    // [Pre-Assert手順] - inet_ntop の失敗時に errno を EINVAL へ設定する。
#elif defined(PLATFORM_WINDOWS)
    // Pre-Assert
    EXPECT_CALL(mock_winsock_, inet_ntop(_, _, _, _, _, _, _))
        .WillOnce(Return(static_cast<PCSTR>(NULL))); // [Pre-Assert確認_異常系] - inet_ntop が NULL を返すこと。
    EXPECT_CALL(mock_winsock_, WSAGetLastError)
        .WillOnce(Return(WSAEINVAL)); // [Pre-Assert確認_異常系] - WSAGetLastError が WSAEINVAL を返すこと。
#endif /* PLATFORM_ */

    // Act
    int rtc = com_util_ipv4_to_string(COM_UTIL_IPV4_ADDR_LOOPBACK, buffer, sizeof(buffer),
                                      &detail); // [手順] - IPv4 文字列化の失敗を注入する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc); // [確認_異常系] - 文字列化に失敗した com_util_ipv4_to_string の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// IPv4 アドレスがドット区切り文字列へ変換されることの確認
TEST_F(endpointTest, to_string_converts_address)
{
    // Arrange
    char buffer[COM_UTIL_IPV4_ADDR_STRLEN] = {};
    com_util_error detail = {};

#if defined(PLATFORM_LINUX)
    NiceMock<Mock_arpa_inet> mock_arpa_inet;

    // Pre-Assert
    EXPECT_CALL(mock_arpa_inet, inet_ntop(_, _, _, _, _, _, _))
        .WillOnce(
            [](const char *, const int, const char *, int, const void *, char *dst, socklen_t)
            {
                std::strcpy(dst, "127.0.0.1");
                return static_cast<const char *>(dst);
            }); // [Pre-Assert確認_正常系] - inet_ntop がループバックアドレスの文字列を返すこと。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_winsock_, inet_ntop(_, _, _, _, _, _, _))
        .WillOnce(
            [](const char *, const int, const char *, INT, const void *, PSTR dst, size_t)
            {
                std::strcpy(dst, "127.0.0.1");
                return static_cast<PCSTR>(dst);
            }); // [Pre-Assert確認_正常系] - inet_ntop がループバックアドレスの文字列を返すこと。
#endif /* PLATFORM_ */

    // Act
    int rtc = com_util_ipv4_to_string(COM_UTIL_IPV4_ADDR_LOOPBACK, buffer, sizeof(buffer),
                                      &detail); // [手順] - IPv4 アドレスを文字列化する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              rtc); // [確認_正常系] - 正常に文字列化した com_util_ipv4_to_string の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("127.0.0.1",
                 buffer); // [確認_正常系] - com_util_ipv4_to_string がドット区切りの IPv4 文字列を返すこと。
    expect_detail(detail, COM_UTIL_ERROR_DOMAIN_NONE, COM_UTIL_OK, 0UL);
}

#if defined(PLATFORM_WINDOWS)
// Winsock の初期化に失敗した場合、IPv4 文字列化が失敗することの確認
TEST_F(endpointTest, to_string_propagates_startup_failure)
{
    // Arrange
    char buffer[COM_UTIL_IPV4_ADDR_STRLEN] = {};
    com_util_error detail = {};

    // Pre-Assert
    EXPECT_CALL(mock_winsock_, WSAStartup(_, _, _, _, _))
        .WillOnce(Return(WSASYSNOTREADY)); // [Pre-Assert確認_異常系] - WSAStartup が初期化失敗を返すこと。

    // Act
    int rtc = com_util_ipv4_to_string(COM_UTIL_IPV4_ADDR_LOOPBACK, buffer, sizeof(buffer),
                                      &detail); // [手順] - 初期化失敗を注入して IPv4 を文字列化する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        rtc); // [確認_異常系] - 初期化に失敗した com_util_ipv4_to_string の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    expect_detail(detail, COM_UTIL_ERROR_DOMAIN_WINSOCK, COM_UTIL_ERR_UNKNOWN,
                  static_cast<unsigned long>(WSASYSNOTREADY));
}
#endif /* PLATFORM_WINDOWS */
