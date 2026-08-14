#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/console/console.h>
#include <com_util/console/console_internal.h>
#include <stdio.h>

/* ===== テスト クラス ===== */

class consoleTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_com_util;

    void SetUp() override
    {
        // console.c の単体テストでは shutdown.c を対象外とし、登録 API の呼び出しをフェイクする。
        ON_CALL(mock_com_util, com_util_shutdown_register(_, _)).WillByDefault(Return(COM_UTIL_OK));
        _mock_com_util = &mock_com_util;
    }

    void TearDown() override
    {
        _mock_com_util = nullptr;
    }
};

/* ===== 共通テスト (Windows / Linux 両方) ===== */

// com_util_console_init がクラッシュしないことの確認
TEST_F(consoleTest, init_succeeds)
{
    // Arrange

    // Pre-Assert

    // Act
    com_util_console_init(); // [手順] - コンソール ヘルパーを初期化する。

    // Assert
    // [確認_正常系] - クラッシュせずに完了すること。
}

// init 後に dispose_on_shutdown() がクラッシュしないことの確認
TEST_F(consoleTest, dispose_on_shutdown_after_init)
{
    // Arrange
    com_util_console_init(); // [状態] - 初期化済みのコンソール ヘルパーとする。
    com_util_shutdown_event event = {COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT, COM_UTIL_SHUTDOWN_CODE_KIND_NONE,
                                     0}; // [状態] - 通常終了イベントを用意する。

    // Pre-Assert

    // Act
    com_util_console_dispose_on_shutdown(&event,
                                         NULL); // [手順] - 正常終了イベントで dispose_on_shutdown() を呼び出す。

    // Assert
    // [確認_正常系] - クラッシュせずに完了すること。
}

// init なしで dispose_on_shutdown() を呼んでも安全なことの確認
TEST_F(consoleTest, dispose_on_shutdown_without_init)
{
    // Arrange
    com_util_shutdown_event event = {COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT, COM_UTIL_SHUTDOWN_CODE_KIND_NONE,
                                     0}; // [状態] - 通常終了イベントを用意する (init は呼ばない)。

    // Pre-Assert

    // Act
    com_util_console_dispose_on_shutdown(&event, NULL); // [手順] - init を呼ばずに dispose_on_shutdown() を呼び出す。

    // Assert
    // [確認_正常系] - 安全に何もせず、クラッシュしないこと。
}

// dispose_on_shutdown() を 2 回呼んでも安全なことの確認
TEST_F(consoleTest, double_dispose_on_shutdown)
{
    // Arrange
    com_util_console_init(); // [状態] - 初期化済みのコンソール ヘルパーとする。
    com_util_shutdown_event event = {COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT, COM_UTIL_SHUTDOWN_CODE_KIND_NONE,
                                     0}; // [状態] - 通常終了イベントを用意する。

    // Pre-Assert

    // Act
    com_util_console_dispose_on_shutdown(&event, NULL); // [手順] - 1 回目の dispose_on_shutdown() を呼び出す。
    com_util_console_dispose_on_shutdown(&event, NULL); // [手順] - 続けて 2 回目の dispose_on_shutdown() を呼び出す。

    // Assert
    // [確認_正常系] - 2 回目は安全に何もせず、クラッシュしないこと。
}

// init 後に終了中イベントの dispose_on_shutdown() が安全に何もしないことの確認
TEST_F(consoleTest, dispose_on_shutdown_process_terminating)
{
    // Arrange
    com_util_console_init(); // [状態] - 初期化済みのコンソール ヘルパーとする。
    com_util_shutdown_event terminating_event = {COM_UTIL_SHUTDOWN_REASON_PROCESS_TERMINATING,
                                                 COM_UTIL_SHUTDOWN_CODE_KIND_NONE,
                                                 0}; // [状態] - プロセス終了中イベントを用意する。
    com_util_shutdown_event normal_event = {COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT, COM_UTIL_SHUTDOWN_CODE_KIND_NONE, 0};

    // Pre-Assert

    // Act
    com_util_console_dispose_on_shutdown(&terminating_event,
                                         NULL); // [手順] - 終了中イベントで dispose_on_shutdown() を呼び出す。

    // Assert
    // [確認_正常系] - 終了中イベントでは何もせず、クラッシュしないこと。

    // Cleanup
    // init 状態を通常終了イベントで解放する。
    com_util_console_dispose_on_shutdown(&normal_event, NULL);
}

// init 後に printf / fprintf を呼んでもクラッシュしないことの確認
TEST_F(consoleTest, write_after_init)
{
    // Arrange
    com_util_console_init(); // [状態] - 初期化済みのコンソール ヘルパーとする。

    // Pre-Assert

    // Act
    printf("consoleTest: stdout\n");          // [手順] - stdout に書き込む。
    fprintf(stderr, "consoleTest: stderr\n"); // [手順] - stderr に書き込む。

    // Assert
    // [確認_正常系] - クラッシュせずに完了すること。
}

/* ===== Linux NOP テスト ===== */
/*
 * Linux では com_util_console_init / com_util_console_dispose は no-op である。
 * 以下のテストは、no-op 実装が stdout / stderr に一切影響を与えないことを確認する。
 */

#if defined(PLATFORM_LINUX)

// Linux: init 前後で stdout の FD が変わらないことの確認
TEST_F(consoleTest, nop_stdout_fd_unchanged)
{
    // Arrange
    int fd_before = fileno(stdout); // [状態] - init 前の stdout FD を記録する。

    // Pre-Assert

    // Act
    com_util_console_init(); // [手順] - コンソール ヘルパーを初期化する。
    int fd_after_init = fileno(stdout);
    com_util_console_dispose(); // [手順] - no-op の dispose を呼び出す。
    int fd_after_dispose = fileno(stdout);

    // Assert
    EXPECT_EQ(fd_before, fd_after_init);    // [確認_正常系] - no-op のため init 後も FD が変わらないこと。
    EXPECT_EQ(fd_before, fd_after_dispose); // [確認_正常系] - dispose 後も FD が変わらないこと。
}

// Linux: init 前後で stderr の FD が変わらないことの確認
TEST_F(consoleTest, nop_stderr_fd_unchanged)
{
    // Arrange
    int fd_before = fileno(stderr); // [状態] - init 前の stderr FD を記録する。

    // Pre-Assert

    // Act
    com_util_console_init(); // [手順] - コンソール ヘルパーを初期化する。
    int fd_after_init = fileno(stderr);
    com_util_console_dispose(); // [手順] - no-op の dispose を呼び出す。
    int fd_after_dispose = fileno(stderr);

    // Assert
    EXPECT_EQ(fd_before, fd_after_init);    // [確認_正常系] - no-op のため init 後も FD が変わらないこと。
    EXPECT_EQ(fd_before, fd_after_dispose); // [確認_正常系] - dispose 後も FD が変わらないこと。
}

// Linux: dispose を呼んでも stdout の FD が変わらないことの確認
TEST_F(consoleTest, nop_dispose_stdout_fd_unchanged)
{
    // Arrange
    int fd_before = fileno(stdout); // [状態] - init 前の stdout FD を記録する。
    com_util_console_init();        // [状態] - 初期化済みのコンソール ヘルパーとする。

    // Pre-Assert

    // Act
    com_util_console_dispose(); // [手順] - no-op の dispose を呼び出す。

    // Assert
    EXPECT_EQ(fd_before, fileno(stdout)); // [確認_正常系] - dispose を呼んでも FD が変わらないこと。
}

#endif /* PLATFORM_LINUX */
