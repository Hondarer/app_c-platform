#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)

    #include <testfw.h>
    #include <mock_com_util.h>
    #include <mock_stdlib.h>
    #include <sys/mock_socket.h>

    #include <com_util/base/result.h>
    #include <com_util/trace/backends/syslog/syslog_internal.h>

    #include <errno.h>
    #include <syslog.h>

using testing::_;
using testing::Assign;
using testing::DoAll;
using testing::DoDefault;
using testing::NiceMock;
using testing::Return;

class syslogFailureInjectionTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_com_util_;
};

// ハンドルの確保に失敗した場合に生成が失敗することの確認
TEST_F(syslogFailureInjectionTest, create_returns_null_when_handle_allocation_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _))
        .WillOnce(Return(nullptr))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - malloc がハンドル確保のために 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - 1 回目は NULL を返却し、以降は本物へ委譲する。

    // Act
    com_util_syslog_sink *handle =
        com_util_syslog_sink_create("syslogFailureInjectionTest",
                                    LOG_USER); // [手順] - com_util_syslog_sink_create を呼び出す。

    // Assert
    EXPECT_EQ((com_util_syslog_sink *)NULL,
              handle); // [確認_異常系] - com_util_syslog_sink_create の戻り値が NULL であること。
}

// 識別子の複製に失敗した場合に生成が失敗することの確認
TEST_F(syslogFailureInjectionTest, create_returns_null_when_ident_duplication_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;

    // Pre-Assert
    /* 1 回目はハンドル、2 回目が識別子の複製になる */
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _))
        .WillOnce(DoDefault())
        .WillOnce(Return(nullptr))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - malloc が識別子の複製のために 2 回目に呼び出されること。
                                      // [Pre-Assert手順] - 2 回目は NULL を返却し、他は本物へ委譲する。

    // Act
    com_util_syslog_sink *handle =
        com_util_syslog_sink_create("syslogFailureInjectionTest",
                                    LOG_USER); // [手順] - com_util_syslog_sink_create を呼び出す。

    // Assert
    EXPECT_EQ((com_util_syslog_sink *)NULL,
              handle); // [確認_異常系] - com_util_syslog_sink_create の戻り値が NULL であること。
}

// ソケットの作成に失敗しても生成が成功することの確認
// Windows は ETW / イベント ログを使うため、この経路は Linux のみに存在する
TEST_F(syslogFailureInjectionTest, create_succeeds_when_socket_creation_fails)
{
    // Arrange
    NiceMock<Mock_sys_socket> mock_sys_socket;

    // Pre-Assert
    EXPECT_CALL(mock_sys_socket, socket(_, _, _, _, _, _))
        .WillOnce(DoAll(Assign(&errno, EMFILE), Return(-1)))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - socket が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - errno に EMFILE を設定し、1 回目は -1 を返却する。

    // Act
    com_util_syslog_sink *handle =
        com_util_syslog_sink_create("syslogFailureInjectionTest",
                                    LOG_USER); // [手順] - com_util_syslog_sink_create を呼び出す。

    // Assert
    EXPECT_NE((com_util_syslog_sink *)NULL,
              handle); // [確認_正常系] - 初回接続に失敗してもハンドルが生成されること。

    // Cleanup
    com_util_syslog_sink_dispose(handle);
}

// 送信に失敗した場合に書き込みが破棄されることの確認
// Windows は ETW / イベント ログを使うため、この経路は Linux のみに存在する
TEST_F(syslogFailureInjectionTest, write_drops_message_when_sendto_fails)
{
    // Arrange
    com_util_syslog_sink *handle = com_util_syslog_sink_create("syslogFailureInjectionTest", LOG_USER);

    ASSERT_NE((com_util_syslog_sink *)NULL, handle); // [状態] - 生成済みのハンドルを用意する。

    NiceMock<Mock_sys_socket> mock_sys_socket;

    // Pre-Assert
    EXPECT_CALL(mock_sys_socket, sendto(_, _, _, _, _, _, _, _, _))
        .WillOnce(DoAll(Assign(&errno, ECONNREFUSED), Return(-1)))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - sendto が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - errno に ECONNREFUSED を設定し、1 回目は -1 を返却する。

    // Act
    int rtc = com_util_syslog_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                         "message"); // [手順] - com_util_syslog_sink_write を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              rtc); // [確認_正常系] - 送信に失敗しても破棄扱いとして com_util_syslog_sink_write は COM_UTIL_OK を返すこと。

    // Cleanup
    com_util_syslog_sink_dispose(handle);
}

// 送信バッファーが満杯の場合に書き込みが破棄されることの確認
// Windows は ETW / イベント ログを使うため、この経路は Linux のみに存在する
TEST_F(syslogFailureInjectionTest, write_drops_message_when_send_buffer_is_full)
{
    // Arrange
    com_util_syslog_sink *handle = com_util_syslog_sink_create("syslogFailureInjectionTest", LOG_USER);

    ASSERT_NE((com_util_syslog_sink *)NULL, handle); // [状態] - 生成済みのハンドルを用意する。

    NiceMock<Mock_sys_socket> mock_sys_socket;

    // Pre-Assert
    EXPECT_CALL(mock_sys_socket, sendto(_, _, _, _, _, _, _, _, _))
        .WillOnce(DoAll(Assign(&errno, EAGAIN), Return(-1)))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - sendto が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - errno に EAGAIN を設定し、1 回目は -1 を返却する。

    // Act
    int rtc = com_util_syslog_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                         "message"); // [手順] - com_util_syslog_sink_write を呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        rtc); // [確認_正常系] - 送信バッファー満杯は再接続を伴わない破棄として COM_UTIL_OK を返すこと。

    // Cleanup
    com_util_syslog_sink_dispose(handle);
}

#endif /* PLATFORM_LINUX */
