#include <testfw.h>
#include <com_util/base/platform.h>
#if defined(PLATFORM_LINUX)
    #include <mock_ioctl.h>
    #include <mock_signal.h>
    #include <mock_termios.h>
    #include <mock_unistd.h>
    #include <sys/mock_select.h>
#endif
#include <mock_com_util.h>
#include <mock_stdio.h>
#include <com_util/base/result.h>
#include <com_util/prompt/pinned_prompt.h>

#include "pinned_prompt.inject.h"

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;
using testing::ReturnArg;
using testing::SetArgPointee;
using testing::SetArrayArgument;

// 固定プロンプトのステータス API が不正なハンドルを分類することの確認
TEST(pinnedPromptTest, status_apis_reject_null_screen)
{
    // Arrange

    // Pre-Assert

    // Act
    int enable_result =
        com_util_pinned_prompt_status_enable(NULL, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_TOP,
                                             1); // [手順] - screen に NULL を渡して上部ステータス領域を有効にする。
    int set_result = com_util_pinned_prompt_status_set(
        NULL, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_TOP, COM_UTIL_PINNED_PROMPT_STATUS_ALIGN_LEFT,
        "status"); // [手順] - screen に NULL を渡して上部左側の表示内容を設定する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        enable_result); // [確認_異常系] - com_util_pinned_prompt_status_enable の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        set_result); // [確認_異常系] - com_util_pinned_prompt_status_set の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// 固定プロンプトの文字列長ヘルパーが NULL を 0 として扱うことの確認
TEST(pinnedPromptTest, static_text_helpers_cover_empty_and_utf8_cases)
{
    // Arrange
    const char combining[] = "\xCC\x81";
    const char cjk[] = "\xE6\x97\xA5";
    const char emoji[] = "\xF0\x9F\x98\x80";
    const char sgr[] = "\x1B[31mX";
    const char invalid_sgr[] = "\x1B[X";

    // Pre-Assert

    // Act
    size_t null_len = test_pinned_prompt_cstr_len(NULL);             // [手順] - NULL の文字列長を取得する。
    size_t ascii_width = test_pinned_prompt_utf8_width("A", 1U, 0U); // [手順] - ASCII の表示幅を取得する。
    size_t combining_width =
        test_pinned_prompt_utf8_width(combining, sizeof(combining) - 1U, 0U); // [手順] - 結合文字の表示幅を取得する。
    size_t cjk_width =
        test_pinned_prompt_utf8_width(cjk, sizeof(cjk) - 1U, 0U); // [手順] - CJK 文字の表示幅を取得する。
    size_t emoji_width =
        test_pinned_prompt_utf8_width(emoji, sizeof(emoji) - 1U, 0U); // [手順] - 4 バイト文字の表示幅を取得する。
    size_t invalid_width =
        test_pinned_prompt_utf8_width("\xC2", 1U, 0U); // [手順] - 不完全な UTF-8 の表示幅を取得する。
    size_t sgr_len = test_pinned_prompt_ansi_len(sgr, sizeof(sgr) - 1U, 0U); // [手順] - ANSI SGR 長を取得する。
    size_t invalid_sgr_len =
        test_pinned_prompt_ansi_len(invalid_sgr, sizeof(invalid_sgr) - 1U, 0U); // [手順] - 不正 SGR 長を取得する。

    // Assert
    EXPECT_EQ(0U, null_len);        // [確認_正常系] - NULL の文字列長が 0 であること。
    EXPECT_EQ(1U, ascii_width);     // [確認_正常系] - ASCII の表示幅が 1 であること。
    EXPECT_EQ(0U, combining_width); // [確認_正常系] - 結合文字の表示幅が 0 であること。
    EXPECT_EQ(2U, cjk_width);       // [確認_正常系] - CJK 文字の表示幅が 2 であること。
    EXPECT_EQ(1U, emoji_width);     // [確認_正常系] - 2FFFF 未満の絵文字の表示幅が 1 であること。
    EXPECT_EQ(1U, invalid_width);   // [確認_正常系] - 不完全な UTF-8 の表示幅が 1 であること。
    EXPECT_EQ(5U, sgr_len);         // [確認_正常系] - ANSI SGR シーケンスの長さが 5 であること。
    EXPECT_EQ(0U, invalid_sgr_len); // [確認_正常系] - 不正な ANSI シーケンスの長さが 0 であること。
}

// 固定プロンプトの表示幅計算が ANSI と全角文字を処理することの確認
TEST(pinnedPromptTest, static_display_helpers_skip_ansi_and_limit_columns)
{
    // Arrange
    const char text[] = "\x1B[31m日A";
    const size_t text_len = sizeof(text) - 1U;

    // Pre-Assert

    // Act
    size_t visible_one_col =
        test_pinned_prompt_visible_bytes(text, text_len, 0U, 1U); // [手順] - 1 列に収まる可視バイト数を取得する。
    size_t visible_three_cols =
        test_pinned_prompt_visible_bytes(text, text_len, 0U, 3U); // [手順] - 3 列に収まる可視バイト数を取得する。
    size_t display_width = test_pinned_prompt_display_width(text, text_len, 0U,
                                                            text_len); // [手順] - ANSI を含む全範囲の表示幅を取得する。
    size_t clipped_width =
        test_pinned_prompt_display_width(text, text_len, 0U, 2U); // [手順] - ANSI 途中までの範囲の表示幅を取得する。

    // Assert
    EXPECT_EQ(5U, visible_one_col);          // [確認_正常系] - SGR と全角文字が 1 列制限で収まること。
    EXPECT_EQ(text_len, visible_three_cols); // [確認_正常系] - 3 列制限で全内容が収まること。
    EXPECT_EQ(3U, display_width);            // [確認_正常系] - 全角文字と ASCII の幅合計が 3 であること。
    EXPECT_EQ(2U, clipped_width);            // [確認_正常系] - ANSI 途中までの範囲が通常文字 2 列として計算されること。
}

// 非 TTY の readline が fgets の入力を改行なしで返すことの確認
TEST(pinnedPromptTest, fallback_readline_strips_newline)
{
    // Arrange
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(NULL);
    ASSERT_NE(nullptr, screen);
    test_pinned_prompt_set_tty(screen, 0);
    NiceMock<Mock_stdio> mock_stdio;
    char input[] = "answer\n";
    char output[32] = {};
    EXPECT_CALL(mock_stdio, fgets(_, _, _, _, _, _))
        .WillOnce(DoAll(SetArrayArgument<3>(input, input + sizeof(input)), ReturnArg<3>()));

    // Pre-Assert

    // Act
    int result = com_util_pinned_prompt_readline(screen, output, sizeof(output),
                                                 NULL); // [手順] - 非 TTY の readline へ改行付き入力を渡す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result);              // [確認_正常系] - fallback readline の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("answer", output); // [確認_正常系] - 出力から改行が除去されること。

    // Cleanup
    com_util_pinned_prompt_dispose(screen);
}

// 非 TTY の readline が EOF を返すことの確認
TEST(pinnedPromptTest, fallback_readline_reports_eof)
{
    // Arrange
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(NULL);
    ASSERT_NE(nullptr, screen);
    test_pinned_prompt_set_tty(screen, 0);
    NiceMock<Mock_stdio> mock_stdio;
    char output[8] = "stale";
    EXPECT_CALL(mock_stdio, fgets(_, _, _, _, _, _)).WillOnce(Return(nullptr));

    // Pre-Assert

    // Act
    int result = com_util_pinned_prompt_readline(screen, output, sizeof(output),
                                                 "prompt"); // [手順] - EOF を返す非 TTY readline を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_EOF,
              result); // [確認_異常系] - EOF の readline が COM_UTIL_ERR_EOF を返すこと。

    // Cleanup
    com_util_pinned_prompt_dispose(screen);
}

// 固定プロンプトのステータス API が上下左右の有効な指定を処理することの確認
TEST(pinnedPromptTest, status_apis_accept_valid_positions_and_alignments)
{
    // Arrange
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(NULL);
    ASSERT_NE(nullptr, screen);

    // Pre-Assert

    // Act
    int top_enable = com_util_pinned_prompt_status_enable(screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_TOP,
                                                          1); // [手順] - 上部ステータスを有効にする。
    int bottom_disable = com_util_pinned_prompt_status_enable(screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_BOTTOM,
                                                              0); // [手順] - 下部ステータスを無効にする。
    int top_left = com_util_pinned_prompt_status_set(screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_TOP,
                                                     COM_UTIL_PINNED_PROMPT_STATUS_ALIGN_LEFT,
                                                     "top"); // [手順] - 上部左側の内容を設定する。
    int top_right = com_util_pinned_prompt_status_set(screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_TOP,
                                                      COM_UTIL_PINNED_PROMPT_STATUS_ALIGN_RIGHT,
                                                      "right"); // [手順] - 上部右側の内容を設定する。
    int bottom_left = com_util_pinned_prompt_status_set(screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_BOTTOM,
                                                        COM_UTIL_PINNED_PROMPT_STATUS_ALIGN_LEFT,
                                                        "bottom"); // [手順] - 下部左側の内容を設定する。
    int bottom_right = com_util_pinned_prompt_status_set(screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_BOTTOM,
                                                         COM_UTIL_PINNED_PROMPT_STATUS_ALIGN_RIGHT,
                                                         NULL); // [手順] - 下部右側の内容を NULL で消去する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, top_enable);     // [確認_正常系] - 上部有効化の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, bottom_disable); // [確認_正常系] - 下部無効化の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, top_left);       // [確認_正常系] - 上部左側設定の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, top_right);      // [確認_正常系] - 上部右側設定の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, bottom_left);    // [確認_正常系] - 下部左側設定の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, bottom_right); // [確認_正常系] - NULL による下部右側消去の戻り値が COM_UTIL_OK であること。

    // Cleanup
    com_util_pinned_prompt_dispose(screen);
}

#if defined(PLATFORM_LINUX)

// 端末サイズ取得が ioctl の成功値とサイズ値を検証することの確認
TEST(pinnedPromptTest, platform_get_size_uses_valid_ioctl_dimensions)
{
    // Arrange
    NiceMock<Mock_ioctl> mock_ioctl;
    struct winsize valid_size = {};
    valid_size.ws_col = 120U;
    valid_size.ws_row = 40U;
    EXPECT_CALL(mock_ioctl, ioctl(_, _, _, STDOUT_FILENO, TIOCGWINSZ, _))
        .WillOnce(DoAll(Invoke([valid_size](const char *, const int, const char *, const int, const unsigned long,
                                            void *arg) { *static_cast<struct winsize *>(arg) = valid_size; }),
                        Return(0)))
        .WillOnce(Return(-1));
    int valid_cols = 0;
    int valid_rows = 0;
    int fallback_cols = 0;
    int fallback_rows = 0;

    // Pre-Assert

    // Act
    test_pinned_prompt_get_size(&valid_cols, &valid_rows);       // [手順] - 正常な ioctl から端末サイズを取得する。
    test_pinned_prompt_get_size(&fallback_cols, &fallback_rows); // [手順] - ioctl 失敗時の既定端末サイズを取得する。

    // Assert
    EXPECT_EQ(120, valid_cols);   // [確認_正常系] - ioctl 成功時の列数が 120 であること。
    EXPECT_EQ(40, valid_rows);    // [確認_正常系] - ioctl 成功時の行数が 40 であること。
    EXPECT_EQ(80, fallback_cols); // [確認_異常系] - ioctl 失敗時の列数が 80 であること。
    EXPECT_EQ(24, fallback_rows); // [確認_異常系] - ioctl 失敗時の行数が 24 であること。
}

// 端末サイズの 0 値が既定値へフォールバックすることの確認
TEST(pinnedPromptTest, platform_get_size_rejects_zero_ioctl_dimensions)
{
    // Arrange
    NiceMock<Mock_ioctl> mock_ioctl;
    struct winsize zero_size = {};
    EXPECT_CALL(mock_ioctl, ioctl(_, _, _, STDOUT_FILENO, TIOCGWINSZ, _))
        .WillOnce(DoAll(Invoke([zero_size](const char *, const int, const char *, const int, const unsigned long,
                                           void *arg) { *static_cast<struct winsize *>(arg) = zero_size; }),
                        Return(0)));
    int cols = 0;
    int rows = 0;

    // Pre-Assert

    // Act
    test_pinned_prompt_get_size(&cols, &rows); // [手順] - 列数と行数が 0 の ioctl 結果を処理する。

    // Assert
    EXPECT_EQ(80, cols); // [確認_異常系] - 列数 0 が既定値 80 へ置き換わること。
    EXPECT_EQ(24, rows); // [確認_異常系] - 行数 0 が既定値 24 へ置き換わること。
}

// Linux の raw モード移行と復帰が端末 API と SIGWINCH API を呼び出すことの確認
TEST(pinnedPromptTest, platform_raw_mode_enters_and_leaves_once)
{
    // Arrange
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(NULL);
    ASSERT_NE(nullptr, screen);
    test_pinned_prompt_reset_platform_state();
    NiceMock<Mock_termios> mock_termios;
    NiceMock<Mock_signal> mock_signal;
    struct termios original = {};
    EXPECT_CALL(mock_termios, tcgetattr(_, _, _, STDIN_FILENO, _))
        .WillOnce(DoAll(SetArgPointee<4>(original), Return(0)));
    EXPECT_CALL(mock_termios, tcsetattr(_, _, _, STDIN_FILENO, _, _)).Times(2).WillRepeatedly(Return(0));
    EXPECT_CALL(mock_signal, sigemptyset(_, _, _, _)).WillOnce(Return(0));
    EXPECT_CALL(mock_signal, sigaction(_, _, _, SIGWINCH, _, _)).Times(2).WillRepeatedly(Return(0));

    // Pre-Assert

    // Act
    test_pinned_prompt_enter_raw(screen); // [手順] - raw モードへ移行する。
    test_pinned_prompt_enter_raw(screen); // [手順] - raw モード移行済みの状態で再度移行する。
    int active_after_enter = test_pinned_prompt_raw_active(screen);
    test_pinned_prompt_leave_raw(screen); // [手順] - raw モードから復帰する。
    test_pinned_prompt_leave_raw(screen); // [手順] - raw モード復帰済みの状態で再度復帰する。
    int active_after_leave = test_pinned_prompt_raw_active(screen);

    // Assert
    EXPECT_EQ(1, active_after_enter); // [確認_正常系] - raw モード移行後の状態が有効であること。
    EXPECT_EQ(0, active_after_leave); // [確認_正常系] - raw モード復帰後の状態が無効であること。

    // Cleanup
    com_util_pinned_prompt_dispose(screen);
}

// Linux の入力読み取りが EINTR とリサイズ通知を分類することの確認
TEST(pinnedPromptTest, platform_read_char_handles_resize_and_eof)
{
    // Arrange
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(NULL);
    ASSERT_NE(nullptr, screen);
    NiceMock<Mock_unistd> mock_unistd;
    unsigned char character = 'B';
    EXPECT_CALL(mock_unistd, read(_, _, _, STDIN_FILENO, _, _))
        .WillOnce(Return(static_cast<ssize_t>(-1)))
        .WillOnce(DoAll(Invoke([character](const char *, const int, const char *, const int, void *arg, const size_t)
                               { *static_cast<unsigned char *>(arg) = character; }),
                        Return(static_cast<ssize_t>(1))))
        .WillOnce(Return(static_cast<ssize_t>(0)));
    test_pinned_prompt_set_resize_pending(1);
    errno = EINTR;

    // Pre-Assert

    // Act
    int resize_result = test_pinned_prompt_read_char(screen); // [手順] - EINTR とリサイズ通知がある状態で入力を読む。
    int character_result = test_pinned_prompt_read_char(screen); // [手順] - 1 バイト入力を読み取る。
    int eof_result = test_pinned_prompt_read_char(screen);       // [手順] - 0 バイト入力を EOF として読み取る。

    // Assert
    EXPECT_EQ(-2, resize_result);                       // [確認_正常系] - リサイズ通知が -2 として返ること。
    EXPECT_EQ(0, test_pinned_prompt_resize_pending());  // [確認_正常系] - 通知状態が消費されること。
    EXPECT_EQ(static_cast<int>('B'), character_result); // [確認_正常系] - 読み取った文字 B が返ること。
    EXPECT_EQ(-1, eof_result);                          // [確認_正常系] - 0 バイト読み取りが EOF として返ること。

    // Cleanup
    com_util_pinned_prompt_dispose(screen);
}

// Linux の non-blocking 入力読み取りが select 結果を処理することの確認
TEST(pinnedPromptTest, platform_read_char_nb_handles_select_timeout)
{
    // Arrange
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(NULL);
    ASSERT_NE(nullptr, screen);
    NiceMock<Mock_sys_select> mock_select;
    NiceMock<Mock_unistd> mock_unistd;
    unsigned char character = 'C';
    EXPECT_CALL(mock_select, select(_, _, _, _, _, _, _, _)).WillOnce(Return(0)).WillOnce(Return(1));
    EXPECT_CALL(mock_unistd, read(_, _, _, STDIN_FILENO, _, _))
        .WillOnce(DoAll(Invoke([character](const char *, const int, const char *, const int, void *arg, const size_t)
                               { *static_cast<unsigned char *>(arg) = character; }),
                        Return(static_cast<ssize_t>(1))));

    // Pre-Assert

    // Act
    int timeout_result =
        test_pinned_prompt_read_char_nb(screen); // [手順] - select がタイムアウトする状態で入力を読む。
    int character_result =
        test_pinned_prompt_read_char_nb(screen); // [手順] - select が入力可能を返す状態で入力を読む。

    // Assert
    EXPECT_EQ(-1, timeout_result);                      // [確認_正常系] - select の 0 が EOF として返ること。
    EXPECT_EQ(static_cast<int>('C'), character_result); // [確認_正常系] - select 後の入力文字 C が返ること。

    // Cleanup
    com_util_pinned_prompt_dispose(screen);
}

// TTY の readline が通常文字を受け取り Enter で入力を確定することの確認
TEST(pinnedPromptTest, tty_readline_accepts_character_and_enter)
{
    // Arrange
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(NULL);
    ASSERT_NE(nullptr, screen);
    test_pinned_prompt_set_tty(screen, 1);
    test_pinned_prompt_reset_platform_state();
    NiceMock<Mock_ioctl> mock_ioctl;
    NiceMock<Mock_signal> mock_signal;
    NiceMock<Mock_termios> mock_termios;
    NiceMock<Mock_unistd> mock_unistd;
    struct termios original = {};
    struct winsize size = {};
    char output[16] = {};
    unsigned char first = 'a';
    unsigned char second = '\n';
    size.ws_col = 80;
    size.ws_row = 24;
    EXPECT_CALL(mock_termios, tcgetattr(_, _, _, STDIN_FILENO, _))
        .WillOnce(DoAll(SetArgPointee<4>(original), Return(0)));
    EXPECT_CALL(mock_termios, tcsetattr(_, _, _, STDIN_FILENO, _, _)).Times(2).WillRepeatedly(Return(0));
    EXPECT_CALL(mock_signal, sigemptyset(_, _, _, _)).WillOnce(Return(0));
    EXPECT_CALL(mock_signal, sigaction(_, _, _, SIGWINCH, _, _)).Times(2).WillRepeatedly(Return(0));
    EXPECT_CALL(mock_ioctl, ioctl(_, _, _, STDOUT_FILENO, TIOCGWINSZ, _))
        .WillRepeatedly(DoAll(Invoke([size](const char *, const int, const char *, const int, const unsigned long,
                                            void *arg) { *static_cast<struct winsize *>(arg) = size; }),
                              Return(0)));
    EXPECT_CALL(mock_unistd, read(_, _, _, STDIN_FILENO, _, _))
        .WillOnce(DoAll(Invoke([first](const char *, const int, const char *, const int, void *arg, const size_t)
                               { *static_cast<unsigned char *>(arg) = first; }),
                        Return(static_cast<ssize_t>(1))))
        .WillOnce(DoAll(Invoke([second](const char *, const int, const char *, const int, void *arg, const size_t)
                               { *static_cast<unsigned char *>(arg) = second; }),
                        Return(static_cast<ssize_t>(1))));

    // Pre-Assert

    // Act
    int result = com_util_pinned_prompt_readline(screen, output, sizeof(output),
                                                 "prompt> "); // [手順] - TTY の readline へ文字 a と Enter を入力する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result);         // [確認_正常系] - TTY の com_util_pinned_prompt_readline が COM_UTIL_OK を返すこと。
    EXPECT_STREQ("a", output); // [確認_正常系] - 確定した入力が "a" であること。

    // Cleanup
    com_util_pinned_prompt_dispose(screen);
}

// TTY の readline が Ctrl-C をキャンセルとして返すことの確認
TEST(pinnedPromptTest, tty_readline_reports_canceled_on_ctrl_c)
{
    // Arrange
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(NULL);
    ASSERT_NE(nullptr, screen);
    test_pinned_prompt_set_tty(screen, 1);
    test_pinned_prompt_reset_platform_state();
    NiceMock<Mock_ioctl> mock_ioctl;
    NiceMock<Mock_signal> mock_signal;
    NiceMock<Mock_termios> mock_termios;
    NiceMock<Mock_unistd> mock_unistd;
    struct termios original = {};
    struct winsize size = {};
    char output[16] = "stale";
    unsigned char cancel = 0x03U;
    size.ws_col = 80;
    size.ws_row = 24;
    EXPECT_CALL(mock_termios, tcgetattr(_, _, _, STDIN_FILENO, _))
        .WillOnce(DoAll(SetArgPointee<4>(original), Return(0)));
    EXPECT_CALL(mock_termios, tcsetattr(_, _, _, STDIN_FILENO, _, _)).Times(2).WillRepeatedly(Return(0));
    EXPECT_CALL(mock_signal, sigemptyset(_, _, _, _)).WillOnce(Return(0));
    EXPECT_CALL(mock_signal, sigaction(_, _, _, SIGWINCH, _, _)).Times(2).WillRepeatedly(Return(0));
    EXPECT_CALL(mock_ioctl, ioctl(_, _, _, STDOUT_FILENO, TIOCGWINSZ, _))
        .WillRepeatedly(DoAll(Invoke([size](const char *, const int, const char *, const int, const unsigned long,
                                            void *arg) { *static_cast<struct winsize *>(arg) = size; }),
                              Return(0)));
    EXPECT_CALL(mock_unistd, read(_, _, _, STDIN_FILENO, _, _))
        .WillOnce(DoAll(Invoke([cancel](const char *, const int, const char *, const int, void *arg, const size_t)
                               { *static_cast<unsigned char *>(arg) = cancel; }),
                        Return(static_cast<ssize_t>(1))));

    // Pre-Assert

    // Act
    int result = com_util_pinned_prompt_readline(screen, output, sizeof(output),
                                                 "prompt> "); // [手順] - TTY の readline へ Ctrl-C を入力する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_CANCELED,
        result); // [確認_異常系] - Ctrl-C の com_util_pinned_prompt_readline が COM_UTIL_ERR_CANCELED を返すこと。
    EXPECT_STREQ("", output); // [確認_異常系] - キャンセル時の出力が空文字列になること。

    // Cleanup
    com_util_pinned_prompt_dispose(screen);
}

#endif /* PLATFORM_LINUX */
