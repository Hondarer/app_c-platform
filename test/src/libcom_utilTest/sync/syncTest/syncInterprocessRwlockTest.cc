#include <testfw.h>

#include "syncTestHelper.h"

// descriptor の出力と復元で同一の interprocess rwlock を再取得できることの確認
TEST(syncInterprocessRwlockTest, descriptor_round_trip_reopens_same_lock)
{
    // Arrange
    char path[256];
    make_test_interprocess_path(path, sizeof(path),
                                "interprocess_rwlock"); // [状態] - テスト用 lock file パスを用意する。
    com_util_interprocess_rwlock *lock = NULL;
    com_util_interprocess_rwlock *restored = NULL;
    unsigned char descriptor[512];
    size_t descriptor_size = sizeof(descriptor);

    // Pre-Assert

    // Act
    int create_result = com_util_interprocess_rwlock_open(path, &lock); // [手順] - interprocess rwlock を開く。
    int export_result = com_util_interprocess_rwlock_export_descriptor(
        lock, descriptor, &descriptor_size); // [手順] - descriptor をバッファーへ出力する。
    int import_result = com_util_interprocess_rwlock_import_descriptor(
        descriptor, descriptor_size, &restored); // [手順] - descriptor から interprocess rwlock を復元する。
    int exclusive_lock = com_util_interprocess_rwlock_lock_exclusive(
        lock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 元ハンドルで排他ロックを取得する。
    int shared_try =
        com_util_interprocess_rwlock_try_lock_shared(restored); // [手順] - 復元ハンドルで共有ロック取得を試行する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        create_result); // [確認_正常系] - com_util_interprocess_rwlock_open の戻り値から、interprocess rwlock open が成功したと判断できること。
    EXPECT_EQ(
        COM_UTIL_OK,
        export_result); // [確認_正常系] - com_util_interprocess_rwlock_export_descriptor の戻り値から、descriptor 出力が成功したと判断できること。
    EXPECT_EQ(COM_UTIL_OK, import_result);    // [確認_正常系] - descriptor から復元できること。
    EXPECT_EQ(COM_UTIL_OK, exclusive_lock);   // [確認_正常系] - 元ハンドルで排他ロックを取得できること。
    EXPECT_EQ(COM_UTIL_ERR_BUSY, shared_try); // [確認_正常系] - 復元ハンドルが同じ排他インスタンスを参照すること。

    // Cleanup
    (void)com_util_interprocess_rwlock_unlock(lock);
    com_util_interprocess_rwlock_destroy(restored);
    com_util_interprocess_rwlock_destroy(lock);
    TEST_INTERPROCESS_UNLINK(path);
}

// NULL バッファーの export で必要な descriptor サイズが報告されることの確認
TEST(syncInterprocessRwlockTest, export_reports_required_descriptor_size)
{
    // Arrange
    char path[256];
    make_test_interprocess_path(path, sizeof(path),
                                "interprocess_rwlock_size"); // [状態] - テスト用 lock file パスを用意する。
    com_util_interprocess_rwlock *lock = NULL;
    size_t descriptor_size = 0U;

    // Pre-Assert

    // Act
    int create_result = com_util_interprocess_rwlock_open(path, &lock); // [手順] - interprocess rwlock を開く。
    int export_result = com_util_interprocess_rwlock_export_descriptor(
        lock, NULL, &descriptor_size); // [手順] - NULL バッファーで必要サイズを問い合わせる。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        create_result); // [確認_正常系] - com_util_interprocess_rwlock_open の戻り値から、interprocess rwlock open が成功したと判断できること。
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL, export_result); // [確認_正常系] - バッファー不足が通知されること。
    EXPECT_GT(descriptor_size, 20U);                         // [確認_正常系] - descriptor に必要なサイズが返ること。

    // Cleanup
    com_util_interprocess_rwlock_destroy(lock);
    TEST_INTERPROCESS_UNLINK(path);
}

// 不正な descriptor の import が拒否されることの確認
TEST(syncInterprocessRwlockTest, corrupt_descriptor_is_rejected)
{
    // Arrange
    unsigned char descriptor[20] = {0}; // [状態] - magic を持たない descriptor を用意する。
    com_util_interprocess_rwlock *lock = NULL;

    // Pre-Assert

    // Act
    int result = com_util_interprocess_rwlock_import_descriptor(descriptor, sizeof(descriptor),
                                                                &lock); // [手順] - 不正 descriptor を import する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_CORRUPT_DESCRIPTOR, result); // [確認_異常系] - 不正 descriptor が拒否されること。
    EXPECT_EQ(NULL, lock);                              // [確認_異常系] - ハンドルが生成されないこと。
}

#if defined(PLATFORM_LINUX)

// fork した子プロセスから親プロセスの rwlock 排他ロックが観測できることの確認
TEST(syncInterprocessRwlockTest, forked_process_observes_exclusive_lock)
{
    // Arrange
    char path[256];
    make_test_interprocess_path(path, sizeof(path),
                                "interprocess_rwlock_fork"); // [状態] - テスト用 lock file パスを用意する。
    com_util_interprocess_rwlock *lock = NULL;

    // Pre-Assert

    // Act
    int open_result = com_util_interprocess_rwlock_open(path, &lock); // [手順] - interprocess rwlock を開く。
    int lock_result = com_util_interprocess_rwlock_lock_exclusive(
        lock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 親プロセスで排他ロックを取得する。
    pid_t child = fork();             // [手順] - fork した子プロセスから同じ lock file を開き共有 try_lock を試行する。

    // Assert
    ASSERT_EQ(
        COM_UTIL_OK,
        open_result); // [確認_正常系] - com_util_interprocess_rwlock_open の戻り値から、interprocess rwlock open が成功したと判断できること。
    ASSERT_EQ(
        COM_UTIL_OK,
        lock_result); // [確認_正常系] - com_util_interprocess_rwlock_lock_exclusive の戻り値から、親プロセスの排他ロック取得が成功したと判断できること。
    ASSERT_GE(child, 0);
    if (child == 0)
    {
        com_util_interprocess_rwlock *child_lock = NULL;
        int child_open = com_util_interprocess_rwlock_open(path, &child_lock);
        int child_try = com_util_interprocess_rwlock_try_lock_shared(child_lock);
        if (child_lock != NULL)
        {

            // Cleanup
            com_util_interprocess_rwlock_destroy(child_lock);
        }
        if (child_open == COM_UTIL_OK && child_try == COM_UTIL_ERR_BUSY)
        {
            _exit(0);
        }
        _exit(1);
    }
    int status = 0;
    ASSERT_EQ(child, waitpid(child, &status, 0));
    EXPECT_TRUE(WIFEXITED(status));    // [確認_正常系] - 子プロセスが正常終了すること。
    EXPECT_EQ(0, WEXITSTATUS(status)); // [確認_正常系] - 子プロセスで open 成功かつ共有 try_lock が BUSY であること。

    // Cleanup
    (void)com_util_interprocess_rwlock_unlock(lock);
    com_util_interprocess_rwlock_destroy(lock);
    TEST_INTERPROCESS_UNLINK(path);
}
#endif /* PLATFORM_LINUX */
