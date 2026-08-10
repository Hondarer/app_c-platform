#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/base/result.h>
#include <com_util/runtime/elevated_process.h>

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
