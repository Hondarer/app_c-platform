#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/base/result.h>
#include <com_util/prompt/pinned_prompt.h>

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
