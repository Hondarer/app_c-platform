#include <testfw.h>

#include "syncTestHelper.h"

// 待機時間 0 ms の指定で即時に戻ることの確認
TEST(syncSleepMsTest, zero_returns_immediately)
{
    // Arrange
    uint64_t before = test_monotonic_ms(); // [状態] - 呼び出し前の単調増加時刻を取得する。

    // Pre-Assert

    // Act
    com_util_sleep_ms(0);                 // [手順] - 待機時間 0 ms で呼び出す。
    uint64_t after = test_monotonic_ms(); // [手順] - 呼び出し後の単調増加時刻を取得する。

    // Assert
    EXPECT_LT(after - before, 100U); // [確認_正常系] - 0 ms 指定で即時に戻ること。
}

// 負の待機時間の指定で即時に戻ることの確認
TEST(syncSleepMsTest, negative_returns_immediately)
{
    // Arrange
    uint64_t before = test_monotonic_ms(); // [状態] - 呼び出し前の単調増加時刻を取得する。

    // Pre-Assert

    // Act
    com_util_sleep_ms(-1);                // [手順] - 負の待機時間で呼び出す。
    uint64_t after = test_monotonic_ms(); // [手順] - 呼び出し後の単調増加時刻を取得する。

    // Assert
    EXPECT_LT(after - before, 100U); // [確認_異常系] - 負値指定で即時に戻ること (no-op)。
}

// 指定した待機時間以上が経過することの確認
TEST(syncSleepMsTest, elapses_at_least_specified_duration)
{
    // Arrange
    const int target_ms = 50;              // [状態] - 待機時間の指示値を 50 ms とする。
    uint64_t before = test_monotonic_ms(); // [状態] - 呼び出し前の単調増加時刻を取得する。

    // Pre-Assert

    // Act
    com_util_sleep_ms(target_ms);         // [手順] - 指定ミリ秒待機する。
    uint64_t after = test_monotonic_ms(); // [手順] - 呼び出し後の単調増加時刻を取得する。

    // Assert
    // Windows の GetTickCount64 は ~15 ms 精度のため余裕を持って判定する。
    EXPECT_GE(after - before, (uint64_t)target_ms - 15U); // [確認_正常系] - 指定ミリ秒以上経過していること。
}
