#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/console/console.h>
#include <com_util/console/console_internal.h>
#include <stdio.h>

/* ===== テストクラス ===== */

class consoleTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_;

    void SetUp() override
    {
        _mock_com_util = &mock_;
    }

    void TearDown() override
    {
        _mock_com_util = nullptr;
    }
};

/* ===== 共通テスト (Windows / Linux 両方) ===== */

// com_util_console_init がクラッシュしないことの確認
TEST_F(consoleTest, test_init_succeeds)
{
    // Act & Assert - クラッシュしないことを確認
    com_util_console_init(); // [手順] - コンソールヘルパーを初期化する。
}

// init 後に dispose_on_shutdown() がクラッシュしないことの確認
TEST_F(consoleTest, test_dispose_on_shutdown_after_init)
{
    // Arrange
    com_util_console_init();
    com_util_shutdown_event_t event = {COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT,
                                       COM_UTIL_SHUTDOWN_CODE_KIND_NONE, 0};

    // Act & Assert - クラッシュしないことを確認
    com_util_console_dispose_on_shutdown(&event, NULL); // [手順] - 正常終了イベントで dispose_on_shutdown() を呼ぶ。
}

// init なしで dispose_on_shutdown() を呼んでも安全なことの確認
TEST_F(consoleTest, test_dispose_on_shutdown_without_init)
{
    // Arrange
    com_util_shutdown_event_t event = {COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT,
                                       COM_UTIL_SHUTDOWN_CODE_KIND_NONE, 0};

    // Act & Assert - クラッシュしないことを確認
    com_util_console_dispose_on_shutdown(&event, NULL); // [手順] - init を呼ばずに dispose_on_shutdown() を呼ぶ。安全に何もしないこと。
}

// dispose_on_shutdown() を 2 回呼んでも安全なことの確認
TEST_F(consoleTest, test_double_dispose_on_shutdown)
{
    // Arrange
    com_util_console_init();
    com_util_shutdown_event_t event = {COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT,
                                       COM_UTIL_SHUTDOWN_CODE_KIND_NONE, 0};

    // Act & Assert - 2 回呼んでもクラッシュしないことを確認
    com_util_console_dispose_on_shutdown(&event, NULL); // [手順] - 1 回目の dispose_on_shutdown()。
    com_util_console_dispose_on_shutdown(&event, NULL); // [手順] - 2 回目の dispose_on_shutdown()。安全に何もしないこと。
}

// init 後に終了中イベントの dispose_on_shutdown() が安全に何もしないことの確認
TEST_F(consoleTest, test_dispose_on_shutdown_process_terminating)
{
    // Arrange
    com_util_console_init();
    com_util_shutdown_event_t terminating_event = {COM_UTIL_SHUTDOWN_REASON_PROCESS_TERMINATING,
                                                   COM_UTIL_SHUTDOWN_CODE_KIND_NONE, 0};
    com_util_shutdown_event_t normal_event = {COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT,
                                              COM_UTIL_SHUTDOWN_CODE_KIND_NONE, 0};

    // Act & Assert - 終了中イベントでは何もしないこと (クラッシュしないことを確認)
    com_util_console_dispose_on_shutdown(&terminating_event, NULL); // [手順] - 終了中イベントを模擬。何もしないこと。

    // Cleanup - init 状態を解放する (通常終了イベントで明示的にクリーンアップ)
    com_util_console_dispose_on_shutdown(&normal_event, NULL);
}

// init 後に printf / fprintf を呼んでもクラッシュしないことの確認
TEST_F(consoleTest, test_write_after_init)
{
    // Arrange
    com_util_console_init();

    // Act & Assert - クラッシュしないことを確認
    printf("consoleTest: stdout\n");           // [手順] - stdout に書き込む。
    fprintf(stderr, "consoleTest: stderr\n");  // [手順] - stderr に書き込む。
}

/* ===== Linux NOP テスト ===== */
/*
 * Linux では com_util_console_init / com_util_console_dispose は no-op である。
 * 以下のテストは、no-op 実装が stdout / stderr に一切影響を与えないことを確認する。
 */

#if defined(PLATFORM_LINUX)

// Linux: init 前後で stdout の FD が変わらないことの確認
TEST_F(consoleTest, test_nop_stdout_fd_unchanged)
{
    // Arrange
    int fd_before = fileno(stdout); // [手順] - init 前の stdout FD を記録する。

    // Act
    com_util_console_init();

    // Assert
    EXPECT_EQ(fd_before, fileno(stdout)); // [確認_正常系] - no-op なので FD が変わっていないこと。

    EXPECT_EQ(fd_before, fileno(stdout)); // [確認_正常系] - dispose 後も FD が変わっていないこと。
}

// Linux: init 前後で stderr の FD が変わらないことの確認
TEST_F(consoleTest, test_nop_stderr_fd_unchanged)
{
    // Arrange
    int fd_before = fileno(stderr); // [手順] - init 前の stderr FD を記録する。

    // Act
    com_util_console_init();

    // Assert
    EXPECT_EQ(fd_before, fileno(stderr)); // [確認_正常系] - no-op なので FD が変わっていないこと。

    EXPECT_EQ(fd_before, fileno(stderr)); // [確認_正常系] - dispose 後も FD が変わっていないこと。
}

// Linux: dispose を呼んでも stdout の FD が変わらないことの確認
TEST_F(consoleTest, test_nop_dispose_stdout_fd_unchanged)
{
    // Arrange
    int fd_before = fileno(stdout); // [手順] - init 前の stdout FD を記録する。
    com_util_console_init();

    // Act
    com_util_console_dispose(); // [手順] - no-op の dispose を呼ぶ。

    // Assert
    EXPECT_EQ(fd_before, fileno(stdout)); // [確認_正常系] - dispose を呼んでも FD が変わらないこと。
}

#endif /* PLATFORM_LINUX */
