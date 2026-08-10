#include <testfw.h>
#include <com_util/base/result.h>
#include <com_util/runtime/elevated_process.h>

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
