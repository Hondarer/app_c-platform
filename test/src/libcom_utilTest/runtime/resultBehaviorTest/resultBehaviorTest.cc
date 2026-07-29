#include <testfw.h>

#include <com_util/console/console.h>
#include <com_util/prompt/pinned_prompt.h>
#include <com_util/runtime/elevated_process.h>
#include <com_util/runtime/sym_loader.h>

// 固定プロンプトのステータス API が不正なハンドルを分類することの確認
TEST(ResultBehaviorTest, PinnedPromptStatusRejectsNullScreen)
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

// シンボル情報 API が空の配列を受け付けることの確認
TEST(ResultBehaviorTest, SymLoaderInfoAcceptsEmptyArray)
{
    // Arrange

    // Pre-Assert

    // Act
    int result = com_util_sym_loader_info(NULL, 0); // [手順] - 要素数 0 と NULL の配列を渡してシンボル情報を表示する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - com_util_sym_loader_info の戻り値が COM_UTIL_OK であること。
}

// シンボル情報 API が不正な配列を拒否することの確認
TEST(ResultBehaviorTest, SymLoaderInfoRejectsInvalidArrays)
{
    // Arrange
    com_util_sym_loader_entry *entries[] = {NULL};

    // Pre-Assert

    // Act
    int null_array_result =
        com_util_sym_loader_info(NULL, 1); // [手順] - 要素数 1 と NULL の配列を渡してシンボル情報を表示する。
    int null_entry_result =
        com_util_sym_loader_info(entries, 1); // [手順] - NULL 要素を含む配列を渡してシンボル情報を表示する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        null_array_result); // [確認_異常系] - 配列が NULL の呼び出しの戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        null_entry_result); // [確認_異常系] - 配列要素が NULL の呼び出しの戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// シンボル情報 API が解決状態を結果コードへ反映することの確認
TEST(ResultBehaviorTest, SymLoaderInfoReportsResolutionState)
{
    // Arrange
    com_util_sym_loader_entry resolved = COM_UTIL_SYM_LOADER_ENTRY_INIT("resolved", void (*)(void));
    com_util_sym_loader_entry unresolved = COM_UTIL_SYM_LOADER_ENTRY_INIT("unresolved", void (*)(void));
    com_util_sym_loader_entry *resolved_entries[] = {&resolved};
    com_util_sym_loader_entry *unresolved_entries[] = {&unresolved};
    resolved.resolved = 1;    // [状態] - 1 個のエントリを解決済みとする。
    unresolved.resolved = -1; // [状態] - 1 個のエントリを解決失敗済みとする。

    // Pre-Assert

    // Act
    int resolved_result = com_util_sym_loader_info(resolved_entries, 1); // [手順] - 解決済みのエントリ情報を表示する。
    int unresolved_result =
        com_util_sym_loader_info(unresolved_entries, 1); // [手順] - 解決失敗済みのエントリ情報を表示する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              resolved_result); // [確認_正常系] - 解決済みエントリに対する戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        unresolved_result); // [確認_異常系] - 解決失敗済みエントリに対する戻り値が COM_UTIL_ERR_UNKNOWN であること。
}

// 親コンソール未接続時に出力フラグが 0 になることの確認
TEST(ResultBehaviorTest, ConsoleAttachInitializesOutput)
{
    // Arrange
    int argc = 1;
    char program[] = "resultBehaviorTest";
    char *argv[] = {program, NULL};
    int attached = 1;

    // Pre-Assert

    // Act
    int result = com_util_console_attach_parent(
        &argc, argv, &attached); // [手順] - 引き継ぎ指定のない引数で親コンソールへの接続を試みる。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result);      // [確認_正常系] - com_util_console_attach_parent の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(0, attached); // [確認_正常系] - attached_out が 0 であること。
}

// 昇格結果報告先が未検出の場合に出力フラグが 0 になることの確認
TEST(ResultBehaviorTest, ElevatedResultTargetInitializesOutput)
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
