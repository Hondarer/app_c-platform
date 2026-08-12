#include <testfw.h>

#include <com_util/base/platform.h>
#include <com_util/base/result.h>
#include <com_util/prompt/pinned_prompt.h>
#include <mock_com_util.h>
#include <mock_stdio.h>
#include <mock_stdlib.h>
#if defined(PLATFORM_LINUX)
    #include <mock_ioctl.h>
    #include <mock_signal.h>
    #include <mock_termios.h>
    #include <mock_unistd.h>
    #include <sys/mock_select.h>
#endif /* PLATFORM_LINUX */

#include "pinned_prompt.inject.h"

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;
using testing::ReturnArg;
using testing::SetArrayArgument;

// UTF-8 と ANSI の境界値を分類することの確認
TEST(pinnedPromptCoverageTest, text_helpers_cover_range_and_ansi_boundaries)
{
    // Arrange
    const char combining_upper_next[] = "\xCD\xB0";
    const char cjk_upper_next[] = "\xEA\x80\x80";
    const char cjk_extension_upper_next[] = "\xE4\xB7\x80";
    const char supplementary_cjk[] = "\xF0\xA0\x80\x80";
    const char not_csi[] = "\x1BXX";
    const char parameter_lower[] = "\x1B[0m";
    const char intermediate_upper[] = "\x1B[/m";
    const char invalid_control[] = "\x1B[\x1F";
    const char invalid_final[] = "\x1B[@";

    // Pre-Assert

    // Act
    size_t combining_upper_next_width =
        test_pinned_prompt_utf8_width(combining_upper_next, sizeof(combining_upper_next) - 1U,
                                      0U); // [手順] - 結合文字範囲の直後にある文字の表示幅を取得する。
    size_t cjk_upper_next_width = test_pinned_prompt_utf8_width(
        cjk_upper_next, sizeof(cjk_upper_next) - 1U, 0U); // [手順] - CJK 統合漢字範囲の直後にある文字の表示幅を取得する。
    size_t cjk_extension_upper_next_width = test_pinned_prompt_utf8_width(
        cjk_extension_upper_next, sizeof(cjk_extension_upper_next) - 1U,
        0U); // [手順] - CJK 拡張 A 範囲の直後にある文字の表示幅を取得する。
    size_t supplementary_cjk_width = test_pinned_prompt_utf8_width(
        supplementary_cjk, sizeof(supplementary_cjk) - 1U, 0U); // [手順] - 補助漢字面の文字の表示幅を取得する。
    size_t not_csi_len =
        test_pinned_prompt_ansi_len(not_csi, sizeof(not_csi) - 1U, 0U); // [手順] - CSI ではない ESC 文字列を解析する。
    size_t parameter_lower_len = test_pinned_prompt_ansi_len(
        parameter_lower, sizeof(parameter_lower) - 1U, 0U); // [手順] - ANSI パラメーター範囲の下限を解析する。
    size_t intermediate_upper_len = test_pinned_prompt_ansi_len(
        intermediate_upper, sizeof(intermediate_upper) - 1U, 0U); // [手順] - ANSI 中間バイト範囲の上限を解析する。
    size_t invalid_control_len = test_pinned_prompt_ansi_len(
        invalid_control, sizeof(invalid_control) - 1U, 0U); // [手順] - ANSI 中間バイト範囲未満の値を解析する。
    size_t invalid_final_len = test_pinned_prompt_ansi_len(
        invalid_final, sizeof(invalid_final) - 1U, 0U); // [手順] - ANSI パラメーター範囲を超える値を解析する。

    // Assert
    EXPECT_EQ(1U, combining_upper_next_width); // [確認_正常系] - 結合文字範囲直後の表示幅が 1 であること。
    EXPECT_EQ(1U, cjk_upper_next_width); // [確認_正常系] - CJK 統合漢字範囲直後の表示幅が 1 であること。
    EXPECT_EQ(1U, cjk_extension_upper_next_width); // [確認_正常系] - CJK 拡張 A 範囲直後の表示幅が 1 であること。
    EXPECT_EQ(2U, supplementary_cjk_width); // [確認_正常系] - 補助漢字面の表示幅が 2 であること。
    EXPECT_EQ(0U, not_csi_len); // [確認_異常系] - CSI ではない ESC 文字列の長さが 0 であること。
    EXPECT_EQ(4U, parameter_lower_len); // [確認_正常系] - ANSI パラメーター下限を含む SGR の長さが 4 であること。
    EXPECT_EQ(4U, intermediate_upper_len); // [確認_正常系] - ANSI 中間バイト上限を含む SGR の長さが 4 であること。
    EXPECT_EQ(0U, invalid_control_len); // [確認_異常系] - ANSI 中間バイト範囲未満の値が拒否されること。
    EXPECT_EQ(0U, invalid_final_len); // [確認_異常系] - ANSI パラメーター範囲を超える値が拒否されること。
}

#if defined(PLATFORM_LINUX)

// Linux 端末判定、サイズ取得、シグナル状態の短絡条件を網羅することの確認
TEST(pinnedPromptCoverageTest, platform_helpers_cover_short_circuits_and_installed_signal)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    NiceMock<Mock_ioctl> mock_ioctl;
    NiceMock<Mock_termios> mock_termios;
    struct winsize column_only_size = {};
    com_util_pinned_prompt *screen = nullptr;
    int cols = 0;
    int rows = 0;
    column_only_size.ws_col = 80U;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_isatty(_))
        .WillOnce(Return(0))
        .WillOnce(Return(1))
        .WillOnce(Return(0))
        .WillOnce(Return(1))
        .WillOnce(Return(1));
    EXPECT_CALL(mock_ioctl, ioctl(_, _, _, STDOUT_FILENO, TIOCGWINSZ, _))
        .WillOnce(DoAll(Invoke([column_only_size](const char *, const int, const char *, const int,
                                                  const unsigned long, void *argument)
                               { *static_cast<struct winsize *>(argument) = column_only_size; }),
                        Return(0)));
    EXPECT_CALL(mock_termios, tcgetattr(_, _, _, STDIN_FILENO, _)).WillOnce(Return(0));
    EXPECT_CALL(mock_termios, tcsetattr(_, _, _, STDIN_FILENO, _, _)).Times(2).WillRepeatedly(Return(0));

    // Act
    int stdin_not_tty = test_pinned_prompt_platform_is_tty(); // [手順] - 標準入力が TTY ではない端末判定を行う。
    int stdout_not_tty = test_pinned_prompt_platform_is_tty(); // [手順] - 標準出力が TTY ではない端末判定を行う。
    int both_tty = test_pinned_prompt_platform_is_tty(); // [手順] - 標準入出力がともに TTY の端末判定を行う。
    test_pinned_prompt_get_size(&cols, &rows); // [手順] - 行数が 0 の ioctl 結果から端末サイズを取得する。
    screen = com_util_pinned_prompt_create(NULL); // [手順] - シグナル登録済み状態の raw モード試験用ハンドルを生成する。
    ASSERT_NE(nullptr, screen);
    test_pinned_prompt_set_tty(screen, 1);
    test_pinned_prompt_set_sigwinch_installed(1);
    test_pinned_prompt_enter_raw(screen); // [手順] - SIGWINCH 登録済み状態で raw モードへ移行する。
    test_pinned_prompt_set_sigwinch_installed(0);
    test_pinned_prompt_leave_raw(screen); // [手順] - SIGWINCH 未登録状態で raw モードから復帰する。
    test_pinned_prompt_raise_resize_handler(); // [手順] - SIGWINCH ハンドラーを直接実行する。

    // Assert
    EXPECT_EQ(0, stdin_not_tty); // [確認_正常系] - 標準入力が TTY ではない場合の端末判定が 0 であること。
    EXPECT_EQ(0, stdout_not_tty); // [確認_正常系] - 標準出力が TTY ではない場合の端末判定が 0 であること。
    EXPECT_EQ(1, both_tty); // [確認_正常系] - 標準入出力がともに TTY の場合の端末判定が 1 であること。
    EXPECT_EQ(80, cols); // [確認_異常系] - 行数が 0 の ioctl 結果では既定列数が返ること。
    EXPECT_EQ(24, rows); // [確認_異常系] - 行数が 0 の ioctl 結果では既定行数が返ること。
    EXPECT_EQ(1, test_pinned_prompt_resize_pending()); // [確認_正常系] - ハンドラー実行後にリサイズ通知が設定されること。

    // Cleanup
    test_pinned_prompt_set_resize_pending(0);
    com_util_pinned_prompt_dispose(screen);
    test_pinned_prompt_reset_platform_state();
}

#endif /* PLATFORM_LINUX */

// ミューテックス無効状態とプロンプト確保失敗を処理することの確認
TEST(pinnedPromptCoverageTest, internal_state_handles_inactive_mutex_and_prompt_failure)
{
    // Arrange
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(NULL);
    ASSERT_NE(nullptr, screen);
    NiceMock<Mock_stdlib> mock_stdlib;
    int null_prompt_result = 0;
    int allocation_result = 0;

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, realloc(_, _, _, _, _)).WillOnce(Return(nullptr));

    // Act
    test_pinned_prompt_set_mutex_active(screen, 0);
    test_pinned_prompt_lock_and_unlock(screen); // [手順] - ミューテックス無効状態でロックとアンロックを行う。
    test_pinned_prompt_set_mutex_active(screen, 1);
    null_prompt_result = test_pinned_prompt_set_prompt(screen, NULL); // [手順] - NULL のプロンプトを設定する。
    allocation_result = test_pinned_prompt_set_prompt(
        screen, "prompt requiring allocation"); // [手順] - 再確保に失敗する長いプロンプトを設定する。

    // Assert
    EXPECT_EQ(0, null_prompt_result); // [確認_正常系] - NULL のプロンプト設定が成功すること。
    EXPECT_EQ(-1, allocation_result); // [確認_異常系] - プロンプト再確保失敗が -1 になること。

    // Cleanup
    com_util_pinned_prompt_dispose(screen);
}

// 表示範囲調整とレイアウトの最小・最大境界を処理することの確認
TEST(pinnedPromptCoverageTest, layout_and_view_cover_narrow_and_status_boundaries)
{
    // Arrange
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(NULL);
    ASSERT_NE(nullptr, screen);
    int prompt_row = 0;
    int separator_row = 0;
    int main_bottom_row = 0;
    int show_top = 0;
    int show_bottom = 0;

    // Pre-Assert

    // Act
    ASSERT_EQ(0, test_pinned_prompt_set_prompt(screen, "long-prompt"));
    test_pinned_prompt_set_edit_line(screen, "abcdef");
    test_pinned_prompt_set_internal_state(screen, 1, 0, 9999, 1, 1, 1, 6U, 4U);
    test_pinned_prompt_set_cursor(screen, 2U);
    test_pinned_prompt_adjust_view(screen); // [手順] - カーソルが表示開始位置より前にある狭幅表示を調整する。
    size_t backward_view = test_pinned_prompt_view_start(screen);
    test_pinned_prompt_set_cursor(screen, 6U);
    test_pinned_prompt_adjust_view(screen); // [手順] - カーソルが表示列数を超える狭幅表示を調整する。
    size_t forward_view = test_pinned_prompt_view_start(screen);
    test_pinned_prompt_layout(screen, &prompt_row, &separator_row, &main_bottom_row, &show_top,
                              &show_bottom); // [手順] - 行数 0 で上下ステータスを有効にしたレイアウトを計算する。
    int minimum_prompt_row = prompt_row;
    int minimum_separator_row = separator_row;
    int minimum_main_bottom_row = main_bottom_row;
    test_pinned_prompt_set_internal_state(screen, 80, 6, 9999, 1, 1, 1, 6U, forward_view);
    test_pinned_prompt_layout(screen, &prompt_row, &separator_row, &main_bottom_row, &show_top,
                              &show_bottom); // [手順] - 上下ステータスを表示できるレイアウトを計算する。

    // Assert
    EXPECT_EQ(2U, backward_view); // [確認_正常系] - 表示開始位置が後退後のカーソル位置になること。
    EXPECT_GT(forward_view, 2U); // [確認_正常系] - 狭幅表示では表示開始位置が前方へ移動すること。
    EXPECT_EQ(1, minimum_prompt_row); // [確認_正常系] - 最小プロンプト行が 1 であること。
    EXPECT_EQ(1, minimum_separator_row); // [確認_正常系] - 最小区切り行が 1 であること。
    EXPECT_EQ(1, minimum_main_bottom_row); // [確認_正常系] - 最小メイン領域末尾行が 1 であること。
    EXPECT_EQ(1, show_top); // [確認_正常系] - 6 行では上部ステータスが表示されること。
    EXPECT_EQ(1, show_bottom); // [確認_正常系] - 6 行では下部ステータスが表示されること。

    // Cleanup
    com_util_pinned_prompt_dispose(screen);
}

#if defined(PLATFORM_LINUX)

// 描画補助が非表示、狭幅、消去範囲の境界を処理することの確認
TEST(pinnedPromptCoverageTest, drawing_helpers_cover_hidden_narrow_and_clear_ranges)
{
    // Arrange
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(NULL);
    ASSERT_NE(nullptr, screen);
    NiceMock<Mock_ioctl> mock_ioctl;
    struct winsize narrow_size = {};
    narrow_size.ws_col = 1U;
    narrow_size.ws_row = 2U;

    // Pre-Assert
    EXPECT_CALL(mock_ioctl, ioctl(_, _, _, STDOUT_FILENO, TIOCGWINSZ, _))
        .WillRepeatedly(DoAll(Invoke([narrow_size](const char *, const int, const char *, const int,
                                                 const unsigned long, void *argument)
                               { *static_cast<struct winsize *>(argument) = narrow_size; }),
                              Return(0)));

    // Act
    test_pinned_prompt_render_state(screen, 1, 0, 1, 1, "prompt", "abcdef", "", "", "123", "456");
    test_pinned_prompt_render(screen); // [手順] - TTY でプロンプト非表示の描画を要求する。
    test_pinned_prompt_hide(screen); // [手順] - TTY でプロンプト非表示の消去を要求する。
    test_pinned_prompt_finish(screen); // [手順] - TTY でプロンプト非表示の確定を要求する。
    test_pinned_prompt_render_state(screen, 1, 1, 1, 1, "prompt", "abcdef", "", "", "123", "456");
    test_pinned_prompt_render(screen); // [手順] - 1 列端末へ長いプロンプトとステータスを描画する。
    test_pinned_prompt_set_internal_state(screen, 1, 2, 0, 1, 1, 1, 6U, 0U);
    test_pinned_prompt_clear_control_area(screen, 1); // [手順] - 前回領域が新しい領域より上にある消去範囲を処理する。
    test_pinned_prompt_set_internal_state(screen, 1, 2, 5, 1, 1, 1, 6U, 0U);
    test_pinned_prompt_clear_control_area(screen, 5); // [手順] - 端末行数を超える消去範囲を処理する。
    test_pinned_prompt_hide(screen); // [手順] - 表示中のプロンプト行を消去する。
    test_pinned_prompt_finish(screen); // [手順] - 表示中のプロンプトを確定する。
    test_pinned_prompt_cleanup_terminal(screen); // [手順] - TTY の制御領域を初期状態へ戻す。

    // Assert
    SUCCEED(); // [確認_正常系] - 非表示、狭幅、消去範囲の各描画処理が完了すること。

    // Cleanup
    com_util_pinned_prompt_dispose(screen);
}

#endif /* PLATFORM_LINUX */

// 履歴の容量拡張、上限循環、NULL 要素を処理することの確認
TEST(pinnedPromptCoverageTest, history_helpers_cover_capacity_and_null_entries)
{
    // Arrange
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(NULL);
    ASSERT_NE(nullptr, screen);

    // Pre-Assert

    // Act
    test_pinned_prompt_history_null_inputs(screen); // [手順] - 履歴追加と編集消去へ NULL と空文字列を渡す。
    test_pinned_prompt_set_input_limits(screen, 3U, 4096U);
    int fill_result = test_pinned_prompt_history_fill(screen, 4U); // [手順] - 履歴上限を超える異なる入力を追加する。
    test_pinned_prompt_history_null_entry_paths(screen); // [手順] - NULL の履歴要素を前後へ参照する。
    int context_result = 0;
    for (int line = 1; line <= 5; line++)
    {
        context_result |= test_pinned_prompt_history_failure_state(screen, "capacity.c", line);
    }
    int context_count = test_pinned_prompt_history_count(screen); // [手順] - 初期容量を超えて生成した履歴コンテキスト数を取得する。

    // Assert
    EXPECT_EQ(0, fill_result); // [確認_正常系] - 履歴上限を超える入力追加が成功すること。
    EXPECT_EQ(0, context_result); // [確認_正常系] - 履歴コンテキストの容量拡張が成功すること。
    EXPECT_EQ(7, context_count); // [確認_正常系] - 生成済み履歴コンテキスト数が 7 であること。

    // Cleanup
    test_pinned_prompt_history_release_entries(screen);
    com_util_pinned_prompt_dispose(screen);
}

// 履歴コンテキストの各確保失敗を分類することの確認
TEST(pinnedPromptCoverageTest, history_context_reports_allocation_failures)
{
    // Arrange
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(NULL);
    ASSERT_NE(nullptr, screen);
    NiceMock<Mock_stdlib> mock_stdlib;
    int realloc_failure = 0;
    int calloc_failure = 0;
    int malloc_failure = 0;

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, realloc(_, _, _, _, _)).WillOnce(Return(nullptr));
    EXPECT_CALL(mock_stdlib, calloc(_, _, _, _, _)).WillOnce(Return(nullptr));
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _)).WillOnce(Return(nullptr));

    // Act
    realloc_failure = test_pinned_prompt_history_failure_state(
        screen, "realloc.c", 1); // [手順] - 履歴コンテキスト配列の再確保を失敗させる。
    calloc_failure = test_pinned_prompt_history_failure_state(
        screen, "calloc.c", 2); // [手順] - 履歴要素配列の確保を失敗させる。
    malloc_failure = test_pinned_prompt_history_failure_state(
        screen, "malloc.c", 3); // [手順] - 履歴保存行の確保を失敗させる。

    // Assert
    EXPECT_EQ(-1, realloc_failure); // [確認_異常系] - 履歴コンテキスト配列の再確保失敗が -1 であること。
    EXPECT_EQ(-1, calloc_failure); // [確認_異常系] - 履歴要素配列の確保失敗が -1 であること。
    EXPECT_EQ(-1, malloc_failure); // [確認_異常系] - 履歴保存行の確保失敗が -1 であること。

    // Cleanup
    com_util_pinned_prompt_dispose(screen);
}

// 入力編集が最大容量と確保失敗を処理することの確認
TEST(pinnedPromptCoverageTest, edit_helpers_preserve_text_when_capacity_is_exhausted)
{
    // Arrange
    com_util_pinned_prompt_options options = {};
    options.input.input_initial_capacity = 2U;
    options.input.input_max_bytes = 2U;
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(&options);
    ASSERT_NE(nullptr, screen);

    // Pre-Assert

    // Act
    test_pinned_prompt_set_edit_line(screen, "long"); // [手順] - 最大容量を超える編集行を設定する。
    test_pinned_prompt_insert_byte(screen, 'X'); // [手順] - 最大容量へ達した編集行へ 1 バイト挿入する。
    test_pinned_prompt_set_cursor(screen, 0U);
    test_pinned_prompt_backspace(screen); // [手順] - カーソル先頭で 1 文字削除を要求する。

    // Assert
    EXPECT_STREQ("l", test_pinned_prompt_edit_text(screen)); // [確認_正常系] - 最大容量内の先頭文字だけが保持されること。
    EXPECT_EQ(1U, test_pinned_prompt_edit_length(screen)); // [確認_正常系] - 編集行の長さが 1 のままであること。

    // Cleanup
    com_util_pinned_prompt_dispose(screen);
}

// 書式バッファーの初回確保、書式処理、再確保の失敗を処理することの確認
TEST(pinnedPromptCoverageTest, format_helper_handles_allocation_and_format_failures)
{
    // Arrange
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(NULL);
    ASSERT_NE(nullptr, screen);
    NiceMock<Mock_stdlib> mock_stdlib;
    NiceMock<Mock_stdio> mock_stdio;
    const char long_text[300] = {};
    int malloc_failure = 0;
    int format_failure = 0;
    int realloc_failure = 0;

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _)).WillOnce(Return(nullptr));
    EXPECT_CALL(mock_stdio, vsnprintf(_, _, _, _, _, _)).WillOnce(Return(-1));
    EXPECT_CALL(mock_stdlib, realloc(_, _, _, _, _)).WillOnce(Return(nullptr));

    // Act
    malloc_failure = test_pinned_prompt_format(screen, "%s", "x"); // [手順] - 書式バッファーの初回確保を失敗させる。
    format_failure = test_pinned_prompt_format(screen, "%s", "x"); // [手順] - 書式処理を失敗させる。
    realloc_failure = test_pinned_prompt_format(screen, "%s", long_text); // [手順] - 長い書式結果の再確保を失敗させる。

    // Assert
    EXPECT_EQ(-1, malloc_failure); // [確認_異常系] - 書式バッファーの初回確保失敗が -1 であること。
    EXPECT_EQ(0, format_failure); // [確認_異常系] - 書式処理失敗が空文字列として処理されること。
    EXPECT_EQ(0, realloc_failure); // [確認_異常系] - 書式バッファー再確保失敗が安全に処理されること。

    // Cleanup
    com_util_pinned_prompt_dispose(screen);
}

// ハンドル生成が構造体、ロック、各文字列の確保失敗を処理することの確認
TEST(pinnedPromptCoverageTest, create_reports_each_resource_failure)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    NiceMock<Mock_com_util> mock_com_util;
    int allocation_call = 0;
    int failure_target = 1;
    com_util_pinned_prompt *calloc_failure = nullptr;
    com_util_pinned_prompt *lock_failure = nullptr;
    com_util_pinned_prompt *allocation_failures[6] = {};

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, calloc(_, _, _, _, _))
        .WillOnce(Return(nullptr))
        .WillRepeatedly(Invoke(delegate_real_calloc));
    EXPECT_CALL(mock_com_util, com_util_local_lock_create(_))
        .WillOnce(Return(COM_UTIL_ERR_OUT_OF_MEMORY))
        .WillRepeatedly(Invoke(delegate_real_com_util_local_lock_create));
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _))
        .WillRepeatedly(Invoke([&allocation_call, &failure_target](const char *file, const int line, const char *func,
                                                                  size_t size)
                               {
                                   allocation_call++;
                                   if (allocation_call == failure_target)
                                   {
                                       return static_cast<void *>(nullptr);
                                   }
                                   return delegate_real_malloc(file, line, func, size);
                               }));

    // Act
    calloc_failure = com_util_pinned_prompt_create(NULL); // [手順] - ハンドル本体の確保を失敗させる。
    lock_failure = com_util_pinned_prompt_create(NULL); // [手順] - ハンドルのロック生成を失敗させる。
    for (int index = 0; index < 6; index++)
    {
        allocation_call = 0;
        failure_target = index + 1;
        allocation_failures[index] = com_util_pinned_prompt_create(NULL);
    }

    // Assert
    EXPECT_EQ(nullptr, calloc_failure); // [確認_異常系] - ハンドル本体の確保失敗で NULL が返ること。
    EXPECT_EQ(nullptr, lock_failure); // [確認_異常系] - ロック生成失敗で NULL が返ること。
    for (com_util_pinned_prompt *screen : allocation_failures)
    {
        EXPECT_EQ(nullptr, screen); // [確認_異常系] - 各文字列バッファーの確保失敗で NULL が返ること。
    }

    // Cleanup
    com_util_pinned_prompt_dispose(NULL);
}

#if defined(PLATFORM_LINUX)

// readline が引数、raw 移行、履歴、プロンプトの失敗を分類することの確認
TEST(pinnedPromptCoverageTest, readline_reports_setup_failures)
{
    // Arrange
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(NULL);
    ASSERT_NE(nullptr, screen);
    NiceMock<Mock_termios> mock_termios;
    NiceMock<Mock_stdio> mock_stdio;
    NiceMock<Mock_stdlib> mock_stdlib;
    char input[] = "fallback\n";
    char output[16] = {};
    int invalid_screen = 0;
    int invalid_buffer = 0;
    int invalid_size = 0;
    int raw_failure = 0;
    int history_failure = 0;
    int prompt_failure = 0;

    // Pre-Assert
    EXPECT_CALL(mock_termios, tcgetattr(_, _, _, STDIN_FILENO, _)).WillOnce(Return(-1));
    EXPECT_CALL(mock_termios, tcsetattr(_, _, _, STDIN_FILENO, _, _)).Times(2).WillRepeatedly(Return(0));
    EXPECT_CALL(mock_stdio, fgets(_, _, _, _, _, _))
        .WillOnce(DoAll(SetArrayArgument<3>(input, input + sizeof(input)), ReturnArg<3>()));
    EXPECT_CALL(mock_stdlib, realloc(_, _, _, _, _)).WillOnce(Return(nullptr)).WillOnce(Return(nullptr));

    // Act
    invalid_screen = _com_util_pinned_prompt_readline(NULL, output, sizeof(output), "", "invalid.c",
                                                       1); // [手順] - NULL ハンドルで readline を呼び出す。
    invalid_buffer = _com_util_pinned_prompt_readline(screen, NULL, sizeof(output), "", "invalid.c",
                                                       2); // [手順] - NULL 出力バッファーで readline を呼び出す。
    invalid_size = _com_util_pinned_prompt_readline(screen, output, 0U, "", "invalid.c",
                                                     3); // [手順] - サイズ 0 の出力バッファーで readline を呼び出す。
    test_pinned_prompt_set_tty(screen, 1);
    raw_failure = _com_util_pinned_prompt_readline(screen, output, sizeof(output), "", "raw.c",
                                                    4); // [手順] - raw モード移行に失敗した readline を呼び出す。
    test_pinned_prompt_set_raw_active(screen, 1);
    history_failure = _com_util_pinned_prompt_readline(screen, output, sizeof(output), "", "history-failure.c",
                                                        5); // [手順] - 履歴コンテキスト確保に失敗した readline を呼び出す。
    ASSERT_EQ(0, test_pinned_prompt_history_failure_state(screen, "prompt-failure.c", 6));
    test_pinned_prompt_set_raw_active(screen, 1);
    prompt_failure = _com_util_pinned_prompt_readline(
        screen, output, sizeof(output), "long prompt", "prompt-failure.c",
        6); // [手順] - プロンプト再確保に失敗した readline を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, invalid_screen); // [確認_異常系] - NULL ハンドルの readline が INVALID_ARGUMENT になること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, invalid_buffer); // [確認_異常系] - NULL 出力バッファーの readline が INVALID_ARGUMENT になること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, invalid_size); // [確認_異常系] - サイズ 0 の readline が INVALID_ARGUMENT になること。
    EXPECT_EQ(COM_UTIL_OK, raw_failure); // [確認_異常系] - raw モード移行失敗時は fallback 入力が成功すること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, history_failure); // [確認_異常系] - 履歴コンテキスト確保失敗が UNKNOWN になること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, prompt_failure); // [確認_異常系] - プロンプト再確保失敗が UNKNOWN になること。

    // Cleanup
    com_util_pinned_prompt_dispose(screen);
}

// readline がクリア、移動、未知キー、切り捨て、EOF を処理することの確認
TEST(pinnedPromptCoverageTest, readline_covers_remaining_key_actions)
{
    // Arrange
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(NULL);
    ASSERT_NE(nullptr, screen);
    test_pinned_prompt_set_tty(screen, 1);
    NiceMock<Mock_ioctl> mock_ioctl;
    NiceMock<Mock_termios> mock_termios;
    NiceMock<Mock_unistd> mock_unistd;
    NiceMock<Mock_sys_select> mock_select;
    struct winsize size = {};
    const unsigned char input[] = {'a', 'b', 0x1BU, '[', 'C', 0x1BU, '[', 'H', 0x1BU, '[', 'F',
                                   0x7FU, 0x1BU, '[', '3', 'x', 0x1BU, 0x01U, '\n', 'a', 'b', 'c', '\n'};
    size_t input_pos = 0U;
    int select_count = 0;
    char first_output[8] = {};
    char short_output[2] = {};
    char eof_output[2] = {};
    size.ws_col = 80U;
    size.ws_row = 24U;

    // Pre-Assert
    EXPECT_CALL(mock_ioctl, ioctl(_, _, _, STDOUT_FILENO, TIOCGWINSZ, _))
        .WillRepeatedly(DoAll(Invoke([size](const char *, const int, const char *, const int, const unsigned long,
                                             void *argument) { *static_cast<struct winsize *>(argument) = size; }),
                              Return(0)));
    EXPECT_CALL(mock_termios, tcsetattr(_, _, _, STDIN_FILENO, _, _)).Times(3).WillRepeatedly(Return(0));
    EXPECT_CALL(mock_unistd, read(_, _, _, STDIN_FILENO, _, _))
        .WillRepeatedly(Invoke([&input, &input_pos](const char *, const int, const char *, const int, void *argument,
                                                   const size_t)
                               {
                                   if (input_pos >= sizeof(input))
                                   {
                                       return static_cast<ssize_t>(0);
                                   }
                                   *static_cast<unsigned char *>(argument) = input[input_pos++];
                                   return static_cast<ssize_t>(1);
                               }));
    EXPECT_CALL(mock_select, select(_, _, _, _, _, _, _, _))
        .WillRepeatedly(Invoke([&select_count](const char *, const int, const char *, int, fd_set *, fd_set *, fd_set *,
                                               struct timeval *)
                               {
                                   select_count++;
                                   return (select_count == 10) ? 0 : 1;
                               }));

    // Act
    test_pinned_prompt_set_raw_active(screen, 1);
    int first_result = _com_util_pinned_prompt_readline(
        screen, first_output, sizeof(first_output), "", "keys.c",
        1); // [手順] - 右移動、Home、End、削除、クリア、未知キーを含む入力を確定する。
    test_pinned_prompt_set_raw_active(screen, 1);
    int short_result = _com_util_pinned_prompt_readline(
        screen, short_output, sizeof(short_output), "", "keys.c",
        2); // [手順] - 出力バッファーより長い入力を確定する。
    test_pinned_prompt_set_raw_active(screen, 1);
    int eof_result = _com_util_pinned_prompt_readline(screen, eof_output, sizeof(eof_output), "", "keys.c",
                                                       3); // [手順] - EOF を受信して入力を終了する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, first_result); // [確認_正常系] - 残りのキー操作を含む readline が OK になること。
    EXPECT_STREQ("", first_output); // [確認_正常系] - クリア後の確定入力が空文字列になること。
    EXPECT_EQ(COM_UTIL_OK, short_result); // [確認_正常系] - 長い入力の readline が OK になること。
    EXPECT_STREQ("a", short_output); // [確認_正常系] - 長い入力が出力容量内へ切り捨てられること。
    EXPECT_EQ(COM_UTIL_ERR_EOF, eof_result); // [確認_正常系] - EOF を受信した readline が EOF になること。

    // Cleanup
    com_util_pinned_prompt_dispose(screen);
}

#endif /* PLATFORM_LINUX */

// TTY 書き込みと printf の失敗経路を分類することの確認
TEST(pinnedPromptCoverageTest, tty_write_and_printf_cover_empty_short_and_failure_paths)
{
    // Arrange
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(NULL);
    ASSERT_NE(nullptr, screen);
    test_pinned_prompt_set_tty(screen, 1);
    NiceMock<Mock_stdio> mock_stdio;
    NiceMock<Mock_stdlib> mock_stdlib;
    const char data[] = "abc";
    size_t written = 0U;

    // Pre-Assert
    EXPECT_CALL(mock_stdio, fwrite(_, _, _, _, _, _, _)).WillOnce(Return(2U)).WillOnce(Return(1U));
    EXPECT_CALL(mock_stdio, vsnprintf(_, _, _, _, _, _)).WillOnce(Return(-1));
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _)).WillOnce(Return(nullptr));

    // Act
    int empty_result = com_util_pinned_prompt_write(
        screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT, NULL, 0U, NULL); // [手順] - TTY へサイズ 0 を書き込む。
    int short_result = com_util_pinned_prompt_write(
        screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT, data, 3U, &written); // [手順] - TTY へ短い書き込みを行う。
    int format_failure = com_util_pinned_prompt_printf(
        screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT, "%s", "x"); // [手順] - printf の書式長計算を失敗させる。
    int malloc_failure = com_util_pinned_prompt_printf(
        screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT, "%s", "x"); // [手順] - printf の出力バッファー確保を失敗させる。
    int write_failure = com_util_pinned_prompt_printf(
        screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT, "%s", "x"); // [手順] - printf の内部書き込みを失敗させる。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, empty_result); // [確認_正常系] - TTY へのサイズ 0 の書き込みが OK になること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, short_result); // [確認_異常系] - TTY への短い書き込みが UNKNOWN になること。
    EXPECT_EQ(2U, written); // [確認_異常系] - TTY の短い書き込みバイト数が 2 であること。
    EXPECT_EQ(-1, format_failure); // [確認_異常系] - printf の書式長計算失敗が -1 になること。
    EXPECT_EQ(-1, malloc_failure); // [確認_異常系] - printf の出力バッファー確保失敗が -1 になること。
    EXPECT_EQ(-1, write_failure); // [確認_異常系] - printf の内部書き込み失敗が -1 になること。

    // Cleanup
    com_util_pinned_prompt_dispose(screen);
}

// ステータスの無効化、有効化、再確保失敗を分類することの確認
TEST(pinnedPromptCoverageTest, status_apis_cover_toggle_and_allocation_failure)
{
    // Arrange
    com_util_pinned_prompt *screen = com_util_pinned_prompt_create(NULL);
    ASSERT_NE(nullptr, screen);
    NiceMock<Mock_stdlib> mock_stdlib;

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, realloc(_, _, _, _, _)).WillOnce(Return(nullptr)).WillOnce(Return(nullptr));

    // Act
    int top_disable = com_util_pinned_prompt_status_enable(
        screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_TOP, 0); // [手順] - 上部ステータスを無効にする。
    int bottom_enable = com_util_pinned_prompt_status_enable(
        screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_BOTTOM, 1); // [手順] - 下部ステータスを有効にする。
    int direct_failure = test_pinned_prompt_set_status_content(
        screen, "long status"); // [手順] - ステータス内容の再確保を直接失敗させる。
    int public_failure = com_util_pinned_prompt_status_set(
        screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_TOP, COM_UTIL_PINNED_PROMPT_STATUS_ALIGN_LEFT,
        "long status"); // [手順] - 公開 API からステータス内容の再確保を失敗させる。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, top_disable); // [確認_正常系] - 上部ステータスの無効化が OK になること。
    EXPECT_EQ(COM_UTIL_OK, bottom_enable); // [確認_正常系] - 下部ステータスの有効化が OK になること。
    EXPECT_EQ(COM_UTIL_ERR_OUT_OF_MEMORY, direct_failure); // [確認_異常系] - 直接のステータス再確保失敗が OUT_OF_MEMORY になること。
    EXPECT_EQ(COM_UTIL_ERR_OUT_OF_MEMORY, public_failure); // [確認_異常系] - 公開 API のステータス再確保失敗が OUT_OF_MEMORY になること。

    // Cleanup
    com_util_pinned_prompt_dispose(screen);
}
