#include <testfw.h>

#include "syncTestHelper.h"

// ロック取得済みの local lock への try_lock が BUSY を報告することの確認
TEST(syncLocalLockTest, try_lock_reports_busy_when_already_locked)
{
    // Arrange
    com_util_local_lock *lock = NULL; // [状態] - 新規 local lock ハンドルの格納先を用意する。

    // Pre-Assert

    // Act
    int create_result = com_util_local_lock_create(&lock);                  // [手順] - local lock を作成する。
    int first_lock = com_util_local_lock_lock(lock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 1 回目のロックを取得する。
    int second_try = com_util_local_lock_try_lock(lock); // [手順] - ロック保持中に try_lock を試行する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        create_result); // [確認_正常系] - com_util_local_lock_create の戻り値から、local lock 作成が成功したと判断できること。
    EXPECT_EQ(
        COM_UTIL_OK,
        first_lock); // [確認_正常系] - com_util_local_lock_lock の戻り値から、1 回目のロック取得が成功したと判断できること。
    EXPECT_TRUE(
        second_try == COM_UTIL_ERR_BUSY ||
        second_try ==
            COM_UTIL_ERR_INVALID_ARGUMENT); // [確認_正常系] - 保持中の try_lock が BUSY (非再帰実装では INVALID_ARGUMENT) を返すこと。

    // Cleanup
    (void)com_util_local_lock_unlock(lock);
    com_util_local_lock_destroy(lock);
}

// 負のタイムアウト指定が INVALID_ARGUMENT になることの確認
TEST(syncLocalLockTest, negative_timeout_returns_invalid_argument)
{
    // Arrange
    com_util_local_lock *lock = NULL; // [状態] - 新規 local lock ハンドルの格納先を用意する。

    // Pre-Assert

    // Act
    int create_result = com_util_local_lock_create(&lock); // [手順] - local lock を作成する。
    int lock_result = com_util_local_lock_lock(lock, -1);  // [手順] - 負のタイムアウト -1 を指定してロックする。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        create_result); // [確認_正常系] - com_util_local_lock_create の戻り値から、local lock 作成が成功したと判断できること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, lock_result); // [確認_異常系] - 負値で INVALID_ARGUMENT を返すこと。

    // Cleanup
    com_util_local_lock_destroy(lock);
}
