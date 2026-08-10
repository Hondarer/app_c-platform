#include <testfw.h>
#include <com_util/prompt/prompt_internal.h>
#include <mock_termios.h>
#include <mock_unistd.h>

#include <errno.h>

#if defined(PLATFORM_LINUX)

    #include <fcntl.h>
    #include <signal.h>
    #include <stdlib.h>
    #include <unistd.h>

    #include "prompt_linux.inject.h"

class promptLinuxTest : public Test
{
  protected:
    com_util_prompt handle_ = {};
    int saved_stdin_ = -1;
    int write_fd_ = -1;
    int master_fd_ = -1;
    unsigned int pad_ = 0; /* 明示的アラインメント */

    void SetUp() override
    {
        saved_stdin_ = dup(STDIN_FILENO);
        ASSERT_LE(0, saved_stdin_);
        test_prompt_set_resize_pending(0);
    }

    void TearDown() override
    {
        if (saved_stdin_ >= 0)
        {
            dup2(saved_stdin_, STDIN_FILENO);
            close(saved_stdin_);
            saved_stdin_ = -1;
        }
        if (write_fd_ >= 0)
        {
            close(write_fd_);
            write_fd_ = -1;
        }
        if (master_fd_ >= 0)
        {
            close(master_fd_);
            master_fd_ = -1;
        }
    }

    /* 標準入力をパイプへ差し替える。書き込み側は write_fd_ に保持する */
    void redirect_stdin_to_pipe()
    {
        int fds[2];

        ASSERT_EQ(0, pipe(fds));
        ASSERT_LE(0, dup2(fds[0], STDIN_FILENO));
        close(fds[0]);
        write_fd_ = fds[1];
    }

    /* 標準入力を疑似端末の従属側へ差し替える。主側は master_fd_ に保持する */
    bool redirect_stdin_to_pty()
    {
        int master = posix_openpt(O_RDWR | O_NOCTTY);

        if (master < 0 || grantpt(master) != 0 || unlockpt(master) != 0)
        {
            if (master >= 0)
            {
                close(master);
            }
            return false;
        }

        const char *slave_name = ptsname(master);
        if (slave_name == NULL)
        {
            close(master);
            return false;
        }

        int slave = open(slave_name, O_RDWR | O_NOCTTY);
        if (slave < 0)
        {
            close(master);
            return false;
        }

        dup2(slave, STDIN_FILENO);
        close(slave);
        master_fd_ = master;
        return true;
    }
};

/*
 * prompt_platform_enter_raw / prompt_platform_leave_raw
 */

// 端末でない標準入力では raw モードへ移行しないことの確認
TEST_F(promptLinuxTest, enter_raw_does_nothing_for_non_terminal)
{
    // Arrange
    redirect_stdin_to_pipe(); // [状態] - 標準入力をパイプへ差し替える。

    // Pre-Assert

    // Act
    prompt_platform_enter_raw(&handle_); // [手順] - prompt_platform_enter_raw を呼び出す。

    // Assert
    EXPECT_EQ(0, handle_.raw_active); // [確認_異常系] - tcgetattr が失敗するため raw モードにならないこと。
}

// 端末に対して raw モードへ移行し復帰できることの確認
TEST_F(promptLinuxTest, enter_and_leave_raw_on_terminal)
{
    // Arrange
    if (!redirect_stdin_to_pty())
    {
        GTEST_SKIP() << "疑似端末を確保できないため実行しません";
    } // [状態] - 標準入力を疑似端末へ差し替える。

    // Pre-Assert

    // Act
    prompt_platform_enter_raw(&handle_); // [手順] - prompt_platform_enter_raw を呼び出す。
    int raw_after_enter = handle_.raw_active;
    int sigwinch_after_enter = test_prompt_sigwinch_installed();
    prompt_platform_leave_raw(&handle_); // [手順] - prompt_platform_leave_raw を呼び出す。

    // Assert
    EXPECT_EQ(1, raw_after_enter);      // [確認_正常系] - raw モードが有効になること。
    EXPECT_EQ(1, sigwinch_after_enter); // [確認_正常系] - SIGWINCH ハンドラーが登録されること。
    EXPECT_EQ(0, handle_.raw_active);   // [確認_正常系] - 復帰後に raw モードが無効になること。
    EXPECT_EQ(0, test_prompt_sigwinch_installed()); // [確認_正常系] - SIGWINCH ハンドラーが復元されること。
}

// 端末設定の適用に失敗した場合に raw モードへ移行しないことの確認
TEST_F(promptLinuxTest, enter_raw_does_nothing_when_tcsetattr_fails)
{
    // Arrange
    NiceMock<Mock_termios> mock_termios;

    if (!redirect_stdin_to_pty())
    {
        GTEST_SKIP() << "疑似端末を確保できないため実行しません";
    } // [状態] - 標準入力を疑似端末へ差し替える。

    // Pre-Assert
    EXPECT_CALL(mock_termios, tcsetattr(_, _, _, STDIN_FILENO, _, _))
        .WillOnce(Return(-1))
        .WillRepeatedly(
            DoDefault()); // [Pre-Assert確認_異常系] - tcsetattr が標準入力を指定して 1 回目に呼び出されること。
                          // [Pre-Assert手順] - 1 回目は -1 を返却し、以降は本物へ委譲する。

    // Act
    prompt_platform_enter_raw(&handle_); // [手順] - prompt_platform_enter_raw を呼び出す。

    // Assert
    EXPECT_EQ(0, handle_.raw_active); // [確認_異常系] - 端末設定を適用できないため raw モードにならないこと。
    EXPECT_EQ(0, test_prompt_sigwinch_installed()); // [確認_異常系] - SIGWINCH ハンドラーが登録されないこと。
}

// raw モード中の再入で二重に移行しないことの確認
TEST_F(promptLinuxTest, enter_raw_is_ignored_while_already_raw)
{
    // Arrange
    handle_.raw_active = 1; // [状態] - すでに raw モードとして扱う。

    // Pre-Assert

    // Act
    prompt_platform_enter_raw(&handle_); // [手順] - raw モード中に prompt_platform_enter_raw を呼び出す。

    // Assert
    EXPECT_EQ(1, handle_.raw_active); // [確認_正常系] - raw モードの状態が変化しないこと。

    // Cleanup
    handle_.raw_active = 0;
}

// raw モードでないときの復帰が何もしないことの確認
TEST_F(promptLinuxTest, leave_raw_is_ignored_when_not_raw)
{
    // Arrange
    handle_.raw_active = 0; // [状態] - raw モードでない状態とする。

    // Pre-Assert

    // Act
    prompt_platform_leave_raw(&handle_); // [手順] - prompt_platform_leave_raw を呼び出す。

    // Assert
    EXPECT_EQ(0, handle_.raw_active); // [確認_正常系] - raw モードの状態が変化しないこと。
}

// SIGWINCH の受信がリサイズ待ちとして記録されることの確認
TEST_F(promptLinuxTest, sigwinch_handler_records_pending_resize)
{
    // Arrange
    if (!redirect_stdin_to_pty())
    {
        GTEST_SKIP() << "疑似端末を確保できないため実行しません";
    }
    prompt_platform_enter_raw(&handle_);
    ASSERT_EQ(1, test_prompt_sigwinch_installed()); // [状態] - SIGWINCH ハンドラーを登録済みの状態にする。

    // Pre-Assert

    // Act
    raise(SIGWINCH); // [手順] - 自プロセスへ SIGWINCH を送出する。

    // Assert
    EXPECT_EQ(1, test_prompt_resize_pending()); // [確認_正常系] - リサイズ待ちとして記録されること。

    // Cleanup
    test_prompt_set_resize_pending(0);
    prompt_platform_leave_raw(&handle_);
}

/*
 * prompt_platform_read_char
 */

// 標準入力から 1 バイトが読み取れることの確認
TEST_F(promptLinuxTest, read_char_returns_next_byte)
{
    // Arrange
    redirect_stdin_to_pipe();
    ASSERT_EQ(1, write(write_fd_, "A", 1u)); // [状態] - パイプへ 'A' を 1 バイト書き込む。

    // Pre-Assert

    // Act
    int rtc = prompt_platform_read_char(&handle_); // [手順] - prompt_platform_read_char を呼び出す。

    // Assert
    EXPECT_EQ('A', rtc); // [確認_正常系] - 読み取ったバイト値 'A' が返ること。
}

// 標準入力が閉じられた場合に EOF が返ることの確認
TEST_F(promptLinuxTest, read_char_returns_minus1_at_eof)
{
    // Arrange
    redirect_stdin_to_pipe();
    close(write_fd_);
    write_fd_ = -1; // [状態] - パイプの書き込み側を閉じて EOF 状態にする。

    // Pre-Assert

    // Act
    int rtc = prompt_platform_read_char(&handle_); // [手順] - prompt_platform_read_char を呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - EOF を示す -1 が返ること。
}

// 端末サイズ変更の通知がリサイズ結果として返ることの確認
TEST_F(promptLinuxTest, read_char_reports_resize_on_interrupted_read)
{
    // Arrange
    NiceMock<Mock_unistd> mock_unistd;

    test_prompt_set_resize_pending(1); // [状態] - SIGWINCH 受信済みの状態にする。

    // Pre-Assert
    EXPECT_CALL(mock_unistd, read(_, _, _, STDIN_FILENO, _, 1u))
        .WillOnce(DoAll(Assign(&errno, EINTR),
                        Return(-1))); // [Pre-Assert確認_異常系] - read が標準入力に対し 1 バイトを指定して 1 回呼び出されること。
                                      // [Pre-Assert手順] - errno に EINTR を設定し、read から -1 を返却する。

    // Act
    int rtc = prompt_platform_read_char(&handle_); // [手順] - prompt_platform_read_char を呼び出す。

    // Assert
    EXPECT_EQ(-2, rtc); // [確認_正常系] - リサイズ通知を示す -2 が返ること。
    EXPECT_EQ(0, test_prompt_sigwinch_installed()); // [確認_正常系] - ハンドラー登録状態は変化しないこと。
}

// 割り込みでリサイズ通知がない場合に読み取りが継続されることの確認
TEST_F(promptLinuxTest, read_char_retries_after_interrupt_without_resize)
{
    // Arrange
    NiceMock<Mock_unistd> mock_unistd;

    test_prompt_set_resize_pending(0); // [状態] - SIGWINCH を受信していない状態にする。

    // Pre-Assert
    EXPECT_CALL(mock_unistd, read(_, _, _, STDIN_FILENO, _, 1u))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return(-1)))
        .WillOnce(DoAll(
            Assign(&errno, EIO),
            Return(-1))); // [Pre-Assert確認_異常系] - read が標準入力に対し 2 回呼び出されること。
                          // [Pre-Assert手順] - 1 回目は errno に EINTR、2 回目は errno に EIO を設定して -1 を返却する。

    // Act
    int rtc = prompt_platform_read_char(&handle_); // [手順] - prompt_platform_read_char を呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - 再試行の結果として -1 が返ること。
}

/*
 * prompt_platform_read_char_nb
 */

// 入力がある場合に 1 バイトが読み取れることの確認
TEST_F(promptLinuxTest, read_char_nb_returns_next_byte_when_available)
{
    // Arrange
    redirect_stdin_to_pipe();
    ASSERT_EQ(1, write(write_fd_, "B", 1u)); // [状態] - パイプへ 'B' を 1 バイト書き込む。

    // Pre-Assert

    // Act
    int rtc = prompt_platform_read_char_nb(&handle_); // [手順] - prompt_platform_read_char_nb を呼び出す。

    // Assert
    EXPECT_EQ('B', rtc); // [確認_正常系] - 読み取ったバイト値 'B' が返ること。
}

// 入力がない場合にタイムアウトすることの確認
TEST_F(promptLinuxTest, read_char_nb_returns_minus1_on_timeout)
{
    // Arrange
    redirect_stdin_to_pipe(); // [状態] - 標準入力をパイプへ差し替え、書き込みは行わない。

    // Pre-Assert

    // Act
    int rtc = prompt_platform_read_char_nb(&handle_); // [手順] - prompt_platform_read_char_nb を呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - 50 ミリ秒待っても入力がないため -1 が返ること。
}

#endif /* PLATFORM_LINUX */
