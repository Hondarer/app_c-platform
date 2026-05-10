#include <testfw.h>

#include <com_util/base/platform.h>
#include <com_util/sync/sync.h>

#include <stdio.h>
#include <string.h>

#if defined(PLATFORM_LINUX)
    #include <unistd.h>
    #include <sys/wait.h>
    static void make_test_interprocess_path(char *buf, size_t size, const char *tag)
    {
        snprintf(buf, size, "/tmp/com_util_%s_%ld.lock", tag, (long)getpid());
    }
    #define TEST_INTERPROCESS_UNLINK(path) unlink(path)
#elif defined(PLATFORM_WINDOWS)
    #include <process.h>
    #include <io.h>
    #include <com_util/base/windows_sdk.h>
    static void make_test_interprocess_path(char *buf, size_t size, const char *tag)
    {
        char tmp_dir[256];
        DWORD len = GetTempPathA((DWORD)sizeof(tmp_dir), tmp_dir);
        if (len == 0)
        {
            tmp_dir[0] = '.';
            tmp_dir[1] = '\0';
            len = 1;
        }
        while (len > 0 && (tmp_dir[len - 1] == '\\' || tmp_dir[len - 1] == '/'))
        {
            tmp_dir[--len] = '\0';
        }
        snprintf(buf, size, "%s/com_util_%s_%ld.lock", tmp_dir, tag, (long)_getpid());
    }
    #define TEST_INTERPROCESS_UNLINK(path) _unlink(path)
#endif

TEST(SyncLocalLockTest, TryLockReportsBusyWhenAlreadyLocked)
{
    // Arrange
    com_util_local_lock_t *lock = NULL;

    // Pre-Assert

    // Act
    com_util_sync_result_t create_result = com_util_local_lock_create(&lock);
    com_util_sync_result_t first_lock = com_util_local_lock_lock(lock, COM_UTIL_SYNC_NO_WAIT);
    com_util_sync_result_t second_try = com_util_local_lock_try_lock(lock);

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, create_result);
    EXPECT_EQ(COM_UTIL_SYNC_OK, first_lock);
    EXPECT_TRUE(second_try == COM_UTIL_SYNC_BUSY || second_try == COM_UTIL_SYNC_INVALID_ARGUMENT);

    (void)com_util_local_lock_unlock(lock);
    com_util_local_lock_destroy(lock);
}

TEST(SyncLocalRwlockTest, SharedLocksCanCoexist)
{
    // Arrange
    com_util_local_rwlock_t *rwlock = NULL; // [状態] - 新規 rwlock ハンドルの格納先を用意する。

    // Pre-Assert

    // Act
    com_util_sync_result_t create_result = com_util_local_rwlock_create(&rwlock); // [手順] - rwlock を作成する。
    com_util_sync_result_t first_lock = com_util_local_rwlock_lock_shared(rwlock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 1 つ目の共有ロックを取得する。
    com_util_sync_result_t second_lock = com_util_local_rwlock_lock_shared(rwlock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 2 つ目の共有ロックを取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, create_result); // [確認_正常系] - rwlock 作成が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, first_lock);    // [確認_正常系] - 1 つ目の共有ロック取得が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, second_lock);   // [確認_正常系] - 共有ロック同士が同時取得できること。

    (void)com_util_local_rwlock_unlock_shared(rwlock);
    (void)com_util_local_rwlock_unlock_shared(rwlock);
    com_util_local_rwlock_destroy(rwlock);
}

TEST(SyncLocalRwlockTest, ExclusiveLockBlocksSharedTryLock)
{
    // Arrange
    com_util_local_rwlock_t *rwlock = NULL; // [状態] - 新規 rwlock ハンドルの格納先を用意する。

    // Pre-Assert

    // Act
    com_util_sync_result_t create_result = com_util_local_rwlock_create(&rwlock); // [手順] - rwlock を作成する。
    com_util_sync_result_t exclusive_lock = com_util_local_rwlock_lock_exclusive(rwlock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 排他ロックを取得する。
    com_util_sync_result_t shared_try = com_util_local_rwlock_try_lock_shared(rwlock); // [手順] - 排他ロック中に共有ロック取得を試行する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, create_result);    // [確認_正常系] - rwlock 作成が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, exclusive_lock);   // [確認_正常系] - 排他ロック取得が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_BUSY, shared_try);     // [確認_正常系] - 排他ロック中の共有 try lock が BUSY を返すこと。

    (void)com_util_local_rwlock_unlock_exclusive(rwlock);
    com_util_local_rwlock_destroy(rwlock);
}

TEST(SyncLocalRwlockTest, WaitingWriterPreventsNewReader)
{
    // Arrange
    com_util_local_rwlock_t *rwlock = NULL; // [状態] - 新規 rwlock ハンドルの格納先を用意する。

    // Pre-Assert

    // Act
    com_util_sync_result_t create_result = com_util_local_rwlock_create(&rwlock); // [手順] - rwlock を作成する。
    com_util_sync_result_t shared_lock = com_util_local_rwlock_lock_shared(rwlock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 先行 reader を取得する。
    com_util_sync_result_t writer_try = com_util_local_rwlock_lock_exclusive(rwlock, 5U); // [手順] - writer 待機を発生させてタイムアウトさせる。
    com_util_sync_result_t second_reader = com_util_local_rwlock_try_lock_shared(rwlock); // [手順] - writer 待機後に新規 reader を試行する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, create_result);       // [確認_正常系] - rwlock 作成が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, shared_lock);         // [確認_正常系] - 先行 reader 取得が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_TIMEOUT, writer_try);     // [確認_正常系] - writer が reader 解放待ちでタイムアウトすること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, second_reader);       // [確認_正常系] - タイムアウトした writer は待機数に残らないこと。

    (void)com_util_local_rwlock_unlock_shared(rwlock);
    (void)com_util_local_rwlock_unlock_shared(rwlock);
    com_util_local_rwlock_destroy(rwlock);
}

TEST(SyncInterprocessLockTest, DescriptorRoundTripReopensSameLock)
{
    // Arrange
    char path[256];
    make_test_interprocess_path(path, sizeof(path), "interprocess_lock");
    com_util_interprocess_lock_t *lock = NULL;
    com_util_interprocess_lock_t *restored = NULL;
    unsigned char descriptor[512];
    size_t descriptor_size = sizeof(descriptor);

    // Pre-Assert

    // Act
    com_util_sync_result_t open_result = com_util_interprocess_lock_open(path, &lock);
    com_util_sync_result_t export_result = com_util_interprocess_lock_export_descriptor(lock, descriptor, &descriptor_size);
    com_util_sync_result_t import_result = com_util_interprocess_lock_import_descriptor(descriptor, descriptor_size, &restored);
    com_util_sync_result_t first_lock = com_util_interprocess_lock_lock(lock, COM_UTIL_SYNC_NO_WAIT);
    com_util_sync_result_t second_try = com_util_interprocess_lock_try_lock(restored);

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, open_result);
    EXPECT_EQ(COM_UTIL_SYNC_OK, export_result);
    EXPECT_EQ(COM_UTIL_SYNC_OK, import_result);
    EXPECT_EQ(COM_UTIL_SYNC_OK, first_lock);
    EXPECT_EQ(COM_UTIL_SYNC_BUSY, second_try);

    (void)com_util_interprocess_lock_unlock(lock);
    com_util_interprocess_lock_destroy(restored);
    com_util_interprocess_lock_destroy(lock);
    TEST_INTERPROCESS_UNLINK(path);
}

TEST(SyncInterprocessLockTest, RejectsRwlockDescriptor)
{
    // Arrange
    char path[256];
    make_test_interprocess_path(path, sizeof(path), "interprocess_lock_kind");
    com_util_interprocess_rwlock_t *rwlock = NULL;
    com_util_interprocess_lock_t *lock = NULL;
    unsigned char descriptor[512];
    size_t descriptor_size = sizeof(descriptor);

    // Pre-Assert

    // Act
    com_util_sync_result_t open_result = com_util_interprocess_rwlock_open(path, &rwlock);
    com_util_sync_result_t export_result = com_util_interprocess_rwlock_export_descriptor(rwlock, descriptor, &descriptor_size);
    com_util_sync_result_t import_result = com_util_interprocess_lock_import_descriptor(descriptor, descriptor_size, &lock);

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, open_result);
    EXPECT_EQ(COM_UTIL_SYNC_OK, export_result);
    EXPECT_EQ(COM_UTIL_SYNC_CORRUPT_DESCRIPTOR, import_result);
    EXPECT_EQ(NULL, lock);

    com_util_interprocess_rwlock_destroy(rwlock);
    TEST_INTERPROCESS_UNLINK(path);
}

TEST(SyncInterprocessRwlockTest, DescriptorRoundTripReopensSameLock)
{
    // Arrange
    char path[256];
    make_test_interprocess_path(path, sizeof(path), "interprocess_rwlock"); // [状態] - テスト用 lock file パスを用意する。
    com_util_interprocess_rwlock_t *lock = NULL;
    com_util_interprocess_rwlock_t *restored = NULL;
    unsigned char descriptor[512];
    size_t descriptor_size = sizeof(descriptor);

    // Pre-Assert

    // Act
    com_util_sync_result_t create_result = com_util_interprocess_rwlock_open(path, &lock); // [手順] - interprocess rwlock を開く。
    com_util_sync_result_t export_result = com_util_interprocess_rwlock_export_descriptor(lock, descriptor, &descriptor_size); // [手順] - descriptor をバッファへ出力する。
    com_util_sync_result_t import_result = com_util_interprocess_rwlock_import_descriptor(descriptor, descriptor_size, &restored); // [手順] - descriptor から interprocess rwlock を復元する。
    com_util_sync_result_t exclusive_lock = com_util_interprocess_rwlock_lock_exclusive(lock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 元ハンドルで排他ロックを取得する。
    com_util_sync_result_t shared_try = com_util_interprocess_rwlock_try_lock_shared(restored); // [手順] - 復元ハンドルで共有ロック取得を試行する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, create_result);       // [確認_正常系] - interprocess rwlock open が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, export_result);       // [確認_正常系] - descriptor 出力が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, import_result);       // [確認_正常系] - descriptor から復元できること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, exclusive_lock);      // [確認_正常系] - 元ハンドルで排他ロックを取得できること。
    EXPECT_EQ(COM_UTIL_SYNC_BUSY, shared_try);        // [確認_正常系] - 復元ハンドルが同じ排他インスタンスを参照すること。

    (void)com_util_interprocess_rwlock_unlock(lock);
    com_util_interprocess_rwlock_destroy(restored);
    com_util_interprocess_rwlock_destroy(lock);
    TEST_INTERPROCESS_UNLINK(path);
}

TEST(SyncInterprocessRwlockTest, ExportReportsRequiredDescriptorSize)
{
    // Arrange
    char path[256];
    make_test_interprocess_path(path, sizeof(path), "interprocess_rwlock_size"); // [状態] - テスト用 lock file パスを用意する。
    com_util_interprocess_rwlock_t *lock = NULL;
    size_t descriptor_size = 0U;

    // Pre-Assert

    // Act
    com_util_sync_result_t create_result = com_util_interprocess_rwlock_open(path, &lock); // [手順] - interprocess rwlock を開く。
    com_util_sync_result_t export_result = com_util_interprocess_rwlock_export_descriptor(lock, NULL, &descriptor_size); // [手順] - NULL バッファで必要サイズを問い合わせる。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, create_result);                    // [確認_正常系] - interprocess rwlock open が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_BUFFER_TOO_SMALL, export_result);      // [確認_正常系] - バッファ不足が通知されること。
    EXPECT_GT(descriptor_size, 20U);                               // [確認_正常系] - descriptor に必要なサイズが返ること。

    com_util_interprocess_rwlock_destroy(lock);
    TEST_INTERPROCESS_UNLINK(path);
}

TEST(SyncInterprocessRwlockTest, CorruptDescriptorIsRejected)
{
    // Arrange
    unsigned char descriptor[20] = {0}; // [状態] - magic を持たない descriptor を用意する。
    com_util_interprocess_rwlock_t *lock = NULL;

    // Pre-Assert

    // Act
    com_util_sync_result_t result = com_util_interprocess_rwlock_import_descriptor(descriptor, sizeof(descriptor), &lock); // [手順] - 不正 descriptor を import する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_CORRUPT_DESCRIPTOR, result); // [確認_異常系] - 不正 descriptor が拒否されること。
    EXPECT_EQ(NULL, lock);                               // [確認_異常系] - ハンドルが生成されないこと。
}

#if defined(PLATFORM_LINUX)
TEST(SyncInterprocessLockTest, ForkedProcessObservesExclusiveLock)
{
    // Arrange
    char path[256];
    make_test_interprocess_path(path, sizeof(path), "interprocess_lock_fork");
    com_util_interprocess_lock_t *lock = NULL;

    // Pre-Assert

    // Act
    com_util_sync_result_t open_result = com_util_interprocess_lock_open(path, &lock);
    com_util_sync_result_t lock_result = com_util_interprocess_lock_lock(lock, COM_UTIL_SYNC_NO_WAIT);
    pid_t child = fork();

    // Assert
    ASSERT_EQ(COM_UTIL_SYNC_OK, open_result);
    ASSERT_EQ(COM_UTIL_SYNC_OK, lock_result);
    ASSERT_GE(child, 0);
    if (child == 0)
    {
        com_util_interprocess_lock_t *child_lock = NULL;
        com_util_sync_result_t child_open = com_util_interprocess_lock_open(path, &child_lock);
        com_util_sync_result_t child_try = com_util_interprocess_lock_try_lock(child_lock);
        if (child_lock != NULL)
        {
            com_util_interprocess_lock_destroy(child_lock);
        }
        _exit((child_open == COM_UTIL_SYNC_OK && child_try == COM_UTIL_SYNC_BUSY) ? 0 : 1);
    }
    int status = 0;
    ASSERT_EQ(child, waitpid(child, &status, 0));
    EXPECT_TRUE(WIFEXITED(status));
    EXPECT_EQ(0, WEXITSTATUS(status));

    (void)com_util_interprocess_lock_unlock(lock);
    com_util_interprocess_lock_destroy(lock);
    TEST_INTERPROCESS_UNLINK(path);
}

TEST(SyncInterprocessRwlockTest, ForkedProcessObservesExclusiveLock)
{
    // Arrange
    char path[256];
    make_test_interprocess_path(path, sizeof(path), "interprocess_rwlock_fork");
    com_util_interprocess_rwlock_t *lock = NULL;

    // Pre-Assert

    // Act
    com_util_sync_result_t open_result = com_util_interprocess_rwlock_open(path, &lock);
    com_util_sync_result_t lock_result = com_util_interprocess_rwlock_lock_exclusive(lock, COM_UTIL_SYNC_NO_WAIT);
    pid_t child = fork();

    // Assert
    ASSERT_EQ(COM_UTIL_SYNC_OK, open_result);
    ASSERT_EQ(COM_UTIL_SYNC_OK, lock_result);
    ASSERT_GE(child, 0);
    if (child == 0)
    {
        com_util_interprocess_rwlock_t *child_lock = NULL;
        com_util_sync_result_t child_open = com_util_interprocess_rwlock_open(path, &child_lock);
        com_util_sync_result_t child_try = com_util_interprocess_rwlock_try_lock_shared(child_lock);
        if (child_lock != NULL)
        {
            com_util_interprocess_rwlock_destroy(child_lock);
        }
        _exit((child_open == COM_UTIL_SYNC_OK && child_try == COM_UTIL_SYNC_BUSY) ? 0 : 1);
    }
    int status = 0;
    ASSERT_EQ(child, waitpid(child, &status, 0));
    EXPECT_TRUE(WIFEXITED(status));
    EXPECT_EQ(0, WEXITSTATUS(status));

    (void)com_util_interprocess_rwlock_unlock(lock);
    com_util_interprocess_rwlock_destroy(lock);
    TEST_INTERPROCESS_UNLINK(path);
}
#endif /* PLATFORM_LINUX */
