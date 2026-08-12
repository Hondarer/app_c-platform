#include <testfw.h>
#include <mock_com_util.h>
#include <mock_unistd.h>
#include <com_util/base/result.h>
#include <com_util/runtime/elevated_process.h>

using testing::_;
using testing::NiceMock;
using testing::Return;

#if defined(PLATFORM_WINDOWS)
    #include <com_util/runtime/process_internal.h>

// このテストではネイティブ プロセスの取り込み経路を実行しないため、リンク用の fake を定義する。
extern "C" com_util_process *com_util_process_adopt_native(intptr_t native_handle)
{
    (void)native_handle;
    return NULL;
}
#endif /* PLATFORM_WINDOWS */

// 昇格結果報告先が未検出の場合に出力フラグが 0 になることの確認
TEST(elevatedProcessTest, elevated_result_target_initializes_output)
{
    // Arrange
    int argc = 1;
    char program[] = "resultBehaviorTest";
    char *argv[] = {program, NULL};
    int detected = 1;

    // Pre-Assert

    // Act
    int result = com_util_elevated_process_extract_result_target(
        &argc, argv, &detected); // [手順] - 結果報告先フラグのない引数から報告先を抽出する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - com_util_elevated_process_extract_result_target の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(0, detected); // [確認_正常系] - detected_out が 0 であること。
}

#if defined(PLATFORM_LINUX)
// Linux の root 判定が geteuid の結果を elevated へ反映することの確認
TEST(elevatedProcessTest, is_elevated_reflects_effective_user_id)
{
    // Arrange
    NiceMock<Mock_unistd> mock_unistd;
    int elevated = -1;
    EXPECT_CALL(mock_unistd, geteuid(_, _, _)).WillOnce(Return(static_cast<uid_t>(0)));

    // Pre-Assert

    // Act
    int root_result =
        com_util_elevated_process_is_elevated(&elevated); // [手順] - geteuid が 0 を返す状態で昇格状態を判定する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              root_result); // [確認_正常系] - root 判定の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(1, elevated); // [確認_正常系] - geteuid が 0 のとき elevated が 1 であること。
}

// Linux の非 root 判定が elevated を 0 にすることの確認
TEST(elevatedProcessTest, is_elevated_rejects_non_root_as_not_elevated)
{
    // Arrange
    NiceMock<Mock_unistd> mock_unistd;
    int elevated = -1;
    EXPECT_CALL(mock_unistd, geteuid(_, _, _)).WillOnce(Return(static_cast<uid_t>(1000)));

    // Pre-Assert

    // Act
    int result =
        com_util_elevated_process_is_elevated(&elevated); // [手順] - geteuid が 1000 を返す状態で昇格状態を判定する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result);      // [確認_正常系] - 非 root 判定の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(0, elevated); // [確認_正常系] - geteuid が 0 以外のとき elevated が 0 であること。
}
#endif /* PLATFORM_LINUX */

// 昇格判定 API が NULL 出力を拒否することの確認
TEST(elevatedProcessTest, elevated_apis_reject_null_outputs)
{
    // Arrange
    int exit_code = -1;
    int handled = -1;

    // Pre-Assert

    // Act
    int is_elevated_result = com_util_elevated_process_is_elevated(NULL); // [手順] - 昇格状態の出力先に NULL を渡す。
    int run_result = com_util_elevated_process_run_if_needed(
        NULL, NULL, &handled); // [手順] - run_if_needed の exit_code に NULL を渡す。
    int run_handled_result = com_util_elevated_process_run_if_needed(
        NULL, &exit_code, NULL); // [手順] - run_if_needed の handled に NULL を渡す。
    int result_run_with_result = com_util_elevated_process_run_with_result(
        NULL, &exit_code, NULL, NULL, 0U); // [手順] - run_with_result の handled に NULL を渡す。
    int result_run_with_null_exit = com_util_elevated_process_run_with_result(
        NULL, NULL, &handled, NULL, 0U); // [手順] - run_with_result の exit_code に NULL を渡す。
    int report_null_result =
        com_util_elevated_process_report_result(NULL); // [手順] - report_result の message に NULL を渡す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              is_elevated_result); // [確認_異常系] - is_elevated の戻り値が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              run_result); // [確認_異常系] - run_if_needed の戻り値が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              run_handled_result); // [確認_異常系] - handled が NULL の run_if_needed が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              result_run_with_result); // [確認_異常系] - run_with_result の戻り値が INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        result_run_with_null_exit); // [確認_異常系] - exit_code が NULL の run_with_result が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              report_null_result); // [確認_異常系] - report_result の戻り値が INVALID_ARGUMENT であること。
}

// Linux 固有 API が省略可能な出力と結果報告を処理することの確認
TEST(elevatedProcessTest, linux_helpers_accept_optional_output_and_report_unavailable_target)
{
    // Arrange
    int argc = 1;
    char program[] = "resultBehaviorTest";
    char *argv[] = {program, NULL};

    // Pre-Assert

    // Act
    int extract_result = com_util_elevated_process_extract_result_target(
        &argc, argv, NULL); // [手順] - detected_out を省略して結果報告先を抽出する。
#if defined(PLATFORM_LINUX)
    int report_result =
        com_util_elevated_process_report_result("result"); // [手順] - Linux で結果メッセージの報告を試行する。
#elif defined(PLATFORM_WINDOWS)
    (void)com_util_elevated_process_report_result(
        "result"); // [手順] - Windows でも結果メッセージの報告呼び出しを実行する (戻り値は未使用)。
#endif /* PLATFORM_ */

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              extract_result); // [確認_正常系] - detected_out を省略した抽出の戻り値が COM_UTIL_OK であること。
#if defined(PLATFORM_LINUX)
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              report_result); // [確認_異常系] - Linux の report_result が COM_UTIL_ERR_UNKNOWN であること。
#endif                        /* PLATFORM_LINUX */
}

#if defined(PLATFORM_LINUX)
// Linux では root の場合だけ昇格処理を完了扱いにすることの確認
TEST(elevatedProcessTest, run_apis_report_linux_elevation_state)
{
    // Arrange
    NiceMock<Mock_unistd> mock_unistd;
    int exit_code = -1;
    int handled = -1;
    char result_message[16] = "stale";
    EXPECT_CALL(mock_unistd, geteuid(_, _, _)).Times(2).WillRepeatedly(Return(static_cast<uid_t>(0)));

    // Pre-Assert

    // Act
    int run_result = com_util_elevated_process_run_if_needed(
        "--test", &exit_code, &handled); // [手順] - root 状態で run_if_needed を実行する。
    int result_run_with_result = com_util_elevated_process_run_with_result(
        "--test", &exit_code, &handled, result_message,
        sizeof(result_message)); // [手順] - root 状態で run_with_result を実行する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              run_result);   // [確認_正常系] - root 状態の run_if_needed が COM_UTIL_OK を返すこと。
    EXPECT_EQ(0, exit_code); // [確認_正常系] - root 状態の run_if_needed が exit_code を 0 にすること。
    EXPECT_EQ(0, handled);   // [確認_正常系] - Linux の run_if_needed が handled を 0 にすること。
    EXPECT_EQ(COM_UTIL_OK,
              result_run_with_result); // [確認_正常系] - root 状態の run_with_result が COM_UTIL_OK を返すこと。
    EXPECT_EQ(0, handled);             // [確認_正常系] - Linux の run_with_result が handled を 0 にすること。
    EXPECT_STREQ("", result_message);  // [確認_正常系] - result_message が初期化されること。
}

// Linux の結果バッファー初期化が NULL とサイズ 0 を受理することの確認
TEST(elevatedProcessTest, run_with_result_accepts_absent_or_zero_sized_message_buffer)
{
    // Arrange
    NiceMock<Mock_unistd> mock_unistd;
    int exit_code = -1;
    int handled = -1;
    char result_message[2] = "x";
    EXPECT_CALL(mock_unistd, geteuid(_, _, _)).Times(2).WillRepeatedly(Return(static_cast<uid_t>(0)));

    // Pre-Assert

    // Act
    int null_buffer_result = com_util_elevated_process_run_with_result(
        NULL, &exit_code, &handled, NULL, 1U); // [手順] - result_message が NULL の状態で実行する。
    int zero_size_result = com_util_elevated_process_run_with_result(
        NULL, &exit_code, &handled, result_message, 0U); // [手順] - result_message_size が 0 の状態で実行する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              null_buffer_result); // [確認_正常系] - NULL バッファー指定時の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              zero_size_result);       // [確認_正常系] - サイズ 0 指定時の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("x", result_message); // [確認_正常系] - サイズ 0 のバッファー内容が変更されないこと。
}

// Linux の非 root 状態では昇格 API が失敗を報告することの確認
TEST(elevatedProcessTest, run_apis_reject_linux_non_root)
{
    // Arrange
    NiceMock<Mock_unistd> mock_unistd;
    int exit_code = 0;
    int handled = 0;
    EXPECT_CALL(mock_unistd, geteuid(_, _, _)).Times(2).WillRepeatedly(Return(static_cast<uid_t>(1000)));

    // Pre-Assert

    // Act
    int run_result = com_util_elevated_process_run_if_needed(
        NULL, &exit_code, &handled); // [手順] - 非 root 状態で run_if_needed を実行する。
    int result_run_with_result = com_util_elevated_process_run_with_result(
        NULL, &exit_code, &handled, NULL, 0U); // [手順] - 非 root 状態で run_with_result を実行する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              run_result);              // [確認_異常系] - 非 root の run_if_needed が UNKNOWN を返すこと。
    EXPECT_EQ(EXIT_FAILURE, exit_code); // [確認_異常系] - 非 root の run_if_needed が失敗コードを返すこと。
    EXPECT_EQ(0, handled);              // [確認_異常系] - 非 root の run_if_needed が handled を 0 にすること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              result_run_with_result);  // [確認_異常系] - 非 root の run_with_result が UNKNOWN を返すこと。
    EXPECT_EQ(EXIT_FAILURE, exit_code); // [確認_異常系] - 非 root の run_with_result が失敗コードを返すこと。
}
#endif /* PLATFORM_LINUX */
