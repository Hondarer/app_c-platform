#include <testfw.h>

#include "syncTestHelper.h"

// descriptor の出力と復元で同一の interprocess lock を再取得できることの確認
TEST(syncInterprocessLockTest, descriptor_round_trip_reopens_same_lock)
{
    // Arrange
    char path[256];
    make_test_interprocess_path(path, sizeof(path),
                                "interprocess_lock"); // [状態] - テスト用 lock file パスを用意する。
    com_util_interprocess_lock *lock = NULL;
    com_util_interprocess_lock *restored = NULL;
    unsigned char descriptor[512];
    size_t descriptor_size = sizeof(descriptor);

    // Pre-Assert

    // Act
    int open_result = com_util_interprocess_lock_open(path, &lock); // [手順] - interprocess lock を開く。
    int export_result = com_util_interprocess_lock_export_descriptor(
        lock, descriptor, &descriptor_size); // [手順] - descriptor をバッファーへ出力する。
    int import_result = com_util_interprocess_lock_import_descriptor(
        descriptor, descriptor_size, &restored); // [手順] - descriptor から interprocess lock を復元する。
    int first_lock =
        com_util_interprocess_lock_lock(lock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 元ハンドルでロックを取得する。
    int second_try = com_util_interprocess_lock_try_lock(restored);   // [手順] - 復元ハンドルで try_lock を試行する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        open_result); // [確認_正常系] - com_util_interprocess_lock_open の戻り値から、interprocess lock open が成功したと判断できること。
    EXPECT_EQ(
        COM_UTIL_OK,
        export_result); // [確認_正常系] - com_util_interprocess_lock_export_descriptor の戻り値から、descriptor 出力が成功したと判断できること。
    EXPECT_EQ(COM_UTIL_OK, import_result);    // [確認_正常系] - descriptor から復元できること。
    EXPECT_EQ(COM_UTIL_OK, first_lock);       // [確認_正常系] - 元ハンドルでロックを取得できること。
    EXPECT_EQ(COM_UTIL_ERR_BUSY, second_try); // [確認_正常系] - 復元ハンドルが同じロック インスタンスを参照すること。

    // Cleanup
    (void)com_util_interprocess_lock_unlock(lock);
    com_util_interprocess_lock_destroy(restored);
    com_util_interprocess_lock_destroy(lock);
    TEST_INTERPROCESS_UNLINK(path);
}

// rwlock の descriptor を interprocess lock として import できないことの確認
TEST(syncInterprocessLockTest, rejects_rwlock_descriptor)
{
    // Arrange
    char path[256];
    make_test_interprocess_path(path, sizeof(path),
                                "interprocess_lock_kind"); // [状態] - テスト用 lock file パスを用意する。
    com_util_interprocess_rwlock *rwlock = NULL;
    com_util_interprocess_lock *lock = NULL;
    unsigned char descriptor[512];
    size_t descriptor_size = sizeof(descriptor);

    // Pre-Assert

    // Act
    int open_result = com_util_interprocess_rwlock_open(path, &rwlock); // [手順] - interprocess rwlock を開く。
    int export_result = com_util_interprocess_rwlock_export_descriptor(
        rwlock, descriptor, &descriptor_size); // [手順] - rwlock の descriptor を出力する。
    int import_result = com_util_interprocess_lock_import_descriptor(
        descriptor, descriptor_size, &lock); // [手順] - rwlock の descriptor を lock として import する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        open_result); // [確認_正常系] - com_util_interprocess_rwlock_open の戻り値から、interprocess rwlock open が成功したと判断できること。
    EXPECT_EQ(
        COM_UTIL_OK,
        export_result); // [確認_正常系] - com_util_interprocess_rwlock_export_descriptor の戻り値から、descriptor 出力が成功したと判断できること。
    EXPECT_EQ(COM_UTIL_ERR_CORRUPT_DESCRIPTOR,
              import_result); // [確認_異常系] - 種別違いの import が CORRUPT_DESCRIPTOR になること。
    EXPECT_EQ(NULL, lock);    // [確認_異常系] - ハンドルが生成されないこと。

    // Cleanup
    com_util_interprocess_rwlock_destroy(rwlock);
    TEST_INTERPROCESS_UNLINK(path);
}

#if defined(PLATFORM_LINUX)

// fork した子プロセスから親プロセスの排他ロックが観測できることの確認
TEST(syncInterprocessLockTest, forked_process_observes_exclusive_lock)
{
    // Arrange
    char path[256];
    make_test_interprocess_path(path, sizeof(path),
                                "interprocess_lock_fork"); // [状態] - テスト用 lock file パスを用意する。
    com_util_interprocess_lock *lock = NULL;

    // Pre-Assert

    // Act
    int open_result = com_util_interprocess_lock_open(path, &lock); // [手順] - interprocess lock を開く。
    int lock_result =
        com_util_interprocess_lock_lock(lock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 親プロセスでロックを取得する。
    pid_t child = fork(); // [手順] - fork した子プロセスから同じ lock file を開き try_lock を試行する。

    // Assert
    ASSERT_EQ(
        COM_UTIL_OK,
        open_result); // [確認_正常系] - com_util_interprocess_lock_open の戻り値から、interprocess lock open が成功したと判断できること。
    ASSERT_EQ(
        COM_UTIL_OK,
        lock_result); // [確認_正常系] - com_util_interprocess_lock_lock の戻り値から、親プロセスのロック取得が成功したと判断できること。
    ASSERT_GE(child, 0);
    if (child == 0)
    {
        com_util_interprocess_lock *child_lock = NULL;
        int child_open = com_util_interprocess_lock_open(path, &child_lock);
        int child_try = com_util_interprocess_lock_try_lock(child_lock);
        if (child_lock != NULL)
        {

            // Cleanup
            com_util_interprocess_lock_destroy(child_lock);
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
    EXPECT_EQ(0, WEXITSTATUS(status)); // [確認_正常系] - 子プロセスで open 成功かつ try_lock が BUSY であること。

    // Cleanup
    (void)com_util_interprocess_lock_unlock(lock);
    com_util_interprocess_lock_destroy(lock);
    TEST_INTERPROCESS_UNLINK(path);
}

#endif /* PLATFORM_LINUX */
