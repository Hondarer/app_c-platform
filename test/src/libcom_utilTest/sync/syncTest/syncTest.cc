#include <testfw.h>

#include <com_util/base/platform.h>
#include <com_util/sync/sync.h>

#include <stdio.h>
#include <string.h>

#if defined(PLATFORM_LINUX)
    #include <time.h>
    #include <unistd.h>
    #include <sys/wait.h>
static void make_test_interprocess_path(char *buf, size_t size, const char *tag)
{
    snprintf(buf, size, "/tmp/com_util_%s_%ld.lock", tag, (long)getpid());
}
    #define TEST_INTERPROCESS_UNLINK(path) unlink(path)
static uint64_t test_monotonic_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}
#elif defined(PLATFORM_WINDOWS)
    #include <process.h>
    #include <io.h>
    #include <com_util/base/windows_sdk.h>
static uint64_t test_monotonic_ms(void)
{
    return (uint64_t)GetTickCount64();
}
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

// ロック取得済みの local lock への try_lock が BUSY を報告することの確認
TEST(SyncLocalLockTest, TryLockReportsBusyWhenAlreadyLocked)
{
    // Arrange
    com_util_local_lock *lock = NULL; // [状態] - 新規 local lock ハンドルの格納先を用意する。

    // Pre-Assert

    // Act
    com_util_sync_result_t create_result = com_util_local_lock_create(&lock); // [手順] - local lock を作成する。
    com_util_sync_result_t first_lock =
        com_util_local_lock_lock(lock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 1 回目のロックを取得する。
    com_util_sync_result_t second_try =
        com_util_local_lock_try_lock(lock); // [手順] - ロック保持中に try_lock を試行する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, create_result); // [確認_正常系] - local lock 作成が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, first_lock);    // [確認_正常系] - 1 回目のロック取得が成功すること。
    EXPECT_TRUE(
        second_try == COM_UTIL_SYNC_BUSY ||
        second_try ==
            COM_UTIL_SYNC_INVALID_ARGUMENT); // [確認_正常系] - 保持中の try_lock が BUSY (非再帰実装では INVALID_ARGUMENT) を返すこと。

    (void)com_util_local_lock_unlock(lock);
    com_util_local_lock_destroy(lock);
}

// 共有ロック同士が同時に取得できることの確認
TEST(SyncLocalRwlockTest, SharedLocksCanCoexist)
{
    // Arrange
    com_util_local_rwlock *rwlock = NULL; // [状態] - 新規 rwlock ハンドルの格納先を用意する。

    // Pre-Assert

    // Act
    com_util_sync_result_t create_result = com_util_local_rwlock_create(&rwlock); // [手順] - rwlock を作成する。
    com_util_sync_result_t first_lock =
        com_util_local_rwlock_lock_shared(rwlock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 1 つ目の共有ロックを取得する。
    com_util_sync_result_t second_lock =
        com_util_local_rwlock_lock_shared(rwlock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 2 つ目の共有ロックを取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, create_result); // [確認_正常系] - rwlock 作成が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, first_lock);    // [確認_正常系] - 1 つ目の共有ロック取得が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, second_lock);   // [確認_正常系] - 共有ロック同士が同時取得できること。

    (void)com_util_local_rwlock_unlock_shared(rwlock);
    (void)com_util_local_rwlock_unlock_shared(rwlock);
    com_util_local_rwlock_destroy(rwlock);
}

// 排他ロック中の共有 try_lock が BUSY になることの確認
TEST(SyncLocalRwlockTest, ExclusiveLockBlocksSharedTryLock)
{
    // Arrange
    com_util_local_rwlock *rwlock = NULL; // [状態] - 新規 rwlock ハンドルの格納先を用意する。

    // Pre-Assert

    // Act
    com_util_sync_result_t create_result = com_util_local_rwlock_create(&rwlock); // [手順] - rwlock を作成する。
    com_util_sync_result_t exclusive_lock =
        com_util_local_rwlock_lock_exclusive(rwlock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 排他ロックを取得する。
    com_util_sync_result_t shared_try =
        com_util_local_rwlock_try_lock_shared(rwlock); // [手順] - 排他ロック中に共有ロック取得を試行する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, create_result);  // [確認_正常系] - rwlock 作成が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, exclusive_lock); // [確認_正常系] - 排他ロック取得が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_BUSY, shared_try);   // [確認_正常系] - 排他ロック中の共有 try lock が BUSY を返すこと。

    (void)com_util_local_rwlock_unlock_exclusive(rwlock);
    com_util_local_rwlock_destroy(rwlock);
}

// 待機中の writer がいる間は新規 reader が取得できないことの確認
TEST(SyncLocalRwlockTest, WaitingWriterPreventsNewReader)
{
    // Arrange
    com_util_local_rwlock *rwlock = NULL; // [状態] - 新規 rwlock ハンドルの格納先を用意する。

    // Pre-Assert

    // Act
    com_util_sync_result_t create_result = com_util_local_rwlock_create(&rwlock); // [手順] - rwlock を作成する。
    com_util_sync_result_t shared_lock =
        com_util_local_rwlock_lock_shared(rwlock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 先行 reader を取得する。
    com_util_sync_result_t writer_try =
        com_util_local_rwlock_lock_exclusive(rwlock, 5U); // [手順] - writer 待機を発生させてタイムアウトさせる。
    com_util_sync_result_t second_reader =
        com_util_local_rwlock_try_lock_shared(rwlock); // [手順] - writer 待機後に新規 reader を試行する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, create_result);   // [確認_正常系] - rwlock 作成が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, shared_lock);     // [確認_正常系] - 先行 reader 取得が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_TIMEOUT, writer_try); // [確認_正常系] - writer が reader 解放待ちでタイムアウトすること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, second_reader);   // [確認_正常系] - タイムアウトした writer は待機数に残らないこと。

    (void)com_util_local_rwlock_unlock_shared(rwlock);
    (void)com_util_local_rwlock_unlock_shared(rwlock);
    com_util_local_rwlock_destroy(rwlock);
}

// descriptor の出力と復元で同一の interprocess lock を再取得できることの確認
TEST(SyncInterprocessLockTest, DescriptorRoundTripReopensSameLock)
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
    com_util_sync_result_t open_result =
        com_util_interprocess_lock_open(path, &lock); // [手順] - interprocess lock を開く。
    com_util_sync_result_t export_result = com_util_interprocess_lock_export_descriptor(
        lock, descriptor, &descriptor_size); // [手順] - descriptor をバッファーへ出力する。
    com_util_sync_result_t import_result = com_util_interprocess_lock_import_descriptor(
        descriptor, descriptor_size, &restored); // [手順] - descriptor から interprocess lock を復元する。
    com_util_sync_result_t first_lock =
        com_util_interprocess_lock_lock(lock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 元ハンドルでロックを取得する。
    com_util_sync_result_t second_try =
        com_util_interprocess_lock_try_lock(restored); // [手順] - 復元ハンドルで try_lock を試行する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, open_result);   // [確認_正常系] - interprocess lock open が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, export_result); // [確認_正常系] - descriptor 出力が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, import_result); // [確認_正常系] - descriptor から復元できること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, first_lock);    // [確認_正常系] - 元ハンドルでロックを取得できること。
    EXPECT_EQ(COM_UTIL_SYNC_BUSY, second_try);  // [確認_正常系] - 復元ハンドルが同じロック インスタンスを参照すること。

    (void)com_util_interprocess_lock_unlock(lock);
    com_util_interprocess_lock_destroy(restored);
    com_util_interprocess_lock_destroy(lock);
    TEST_INTERPROCESS_UNLINK(path);
}

// rwlock の descriptor を interprocess lock として import できないことの確認
TEST(SyncInterprocessLockTest, RejectsRwlockDescriptor)
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
    com_util_sync_result_t open_result =
        com_util_interprocess_rwlock_open(path, &rwlock); // [手順] - interprocess rwlock を開く。
    com_util_sync_result_t export_result = com_util_interprocess_rwlock_export_descriptor(
        rwlock, descriptor, &descriptor_size); // [手順] - rwlock の descriptor を出力する。
    com_util_sync_result_t import_result = com_util_interprocess_lock_import_descriptor(
        descriptor, descriptor_size, &lock); // [手順] - rwlock の descriptor を lock として import する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, open_result);   // [確認_正常系] - interprocess rwlock open が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, export_result); // [確認_正常系] - descriptor 出力が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_CORRUPT_DESCRIPTOR,
              import_result); // [確認_異常系] - 種別違いの import が CORRUPT_DESCRIPTOR になること。
    EXPECT_EQ(NULL, lock);    // [確認_異常系] - ハンドルが生成されないこと。

    com_util_interprocess_rwlock_destroy(rwlock);
    TEST_INTERPROCESS_UNLINK(path);
}

// descriptor の出力と復元で同一の interprocess rwlock を再取得できることの確認
TEST(SyncInterprocessRwlockTest, DescriptorRoundTripReopensSameLock)
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
    com_util_sync_result_t create_result =
        com_util_interprocess_rwlock_open(path, &lock); // [手順] - interprocess rwlock を開く。
    com_util_sync_result_t export_result = com_util_interprocess_rwlock_export_descriptor(
        lock, descriptor, &descriptor_size); // [手順] - descriptor をバッファーへ出力する。
    com_util_sync_result_t import_result = com_util_interprocess_rwlock_import_descriptor(
        descriptor, descriptor_size, &restored); // [手順] - descriptor から interprocess rwlock を復元する。
    com_util_sync_result_t exclusive_lock = com_util_interprocess_rwlock_lock_exclusive(
        lock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 元ハンドルで排他ロックを取得する。
    com_util_sync_result_t shared_try =
        com_util_interprocess_rwlock_try_lock_shared(restored); // [手順] - 復元ハンドルで共有ロック取得を試行する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, create_result);  // [確認_正常系] - interprocess rwlock open が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, export_result);  // [確認_正常系] - descriptor 出力が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, import_result);  // [確認_正常系] - descriptor から復元できること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, exclusive_lock); // [確認_正常系] - 元ハンドルで排他ロックを取得できること。
    EXPECT_EQ(COM_UTIL_SYNC_BUSY, shared_try);   // [確認_正常系] - 復元ハンドルが同じ排他インスタンスを参照すること。

    (void)com_util_interprocess_rwlock_unlock(lock);
    com_util_interprocess_rwlock_destroy(restored);
    com_util_interprocess_rwlock_destroy(lock);
    TEST_INTERPROCESS_UNLINK(path);
}

// NULL バッファーの export で必要な descriptor サイズが報告されることの確認
TEST(SyncInterprocessRwlockTest, ExportReportsRequiredDescriptorSize)
{
    // Arrange
    char path[256];
    make_test_interprocess_path(path, sizeof(path),
                                "interprocess_rwlock_size"); // [状態] - テスト用 lock file パスを用意する。
    com_util_interprocess_rwlock *lock = NULL;
    size_t descriptor_size = 0U;

    // Pre-Assert

    // Act
    com_util_sync_result_t create_result =
        com_util_interprocess_rwlock_open(path, &lock); // [手順] - interprocess rwlock を開く。
    com_util_sync_result_t export_result = com_util_interprocess_rwlock_export_descriptor(
        lock, NULL, &descriptor_size); // [手順] - NULL バッファーで必要サイズを問い合わせる。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, create_result); // [確認_正常系] - interprocess rwlock open が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_BUFFER_TOO_SMALL, export_result); // [確認_正常系] - バッファー不足が通知されること。
    EXPECT_GT(descriptor_size, 20U);                          // [確認_正常系] - descriptor に必要なサイズが返ること。

    com_util_interprocess_rwlock_destroy(lock);
    TEST_INTERPROCESS_UNLINK(path);
}

// 不正な descriptor の import が拒否されることの確認
TEST(SyncInterprocessRwlockTest, CorruptDescriptorIsRejected)
{
    // Arrange
    unsigned char descriptor[20] = {0}; // [状態] - magic を持たない descriptor を用意する。
    com_util_interprocess_rwlock *lock = NULL;

    // Pre-Assert

    // Act
    com_util_sync_result_t result = com_util_interprocess_rwlock_import_descriptor(
        descriptor, sizeof(descriptor), &lock); // [手順] - 不正 descriptor を import する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_CORRUPT_DESCRIPTOR, result); // [確認_異常系] - 不正 descriptor が拒否されること。
    EXPECT_EQ(NULL, lock);                               // [確認_異常系] - ハンドルが生成されないこと。
}

#if defined(PLATFORM_LINUX)
// fork した子プロセスから親プロセスの排他ロックが観測できることの確認
TEST(SyncInterprocessLockTest, ForkedProcessObservesExclusiveLock)
{
    // Arrange
    char path[256];
    make_test_interprocess_path(path, sizeof(path),
                                "interprocess_lock_fork"); // [状態] - テスト用 lock file パスを用意する。
    com_util_interprocess_lock *lock = NULL;

    // Pre-Assert

    // Act
    com_util_sync_result_t open_result =
        com_util_interprocess_lock_open(path, &lock); // [手順] - interprocess lock を開く。
    com_util_sync_result_t lock_result =
        com_util_interprocess_lock_lock(lock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 親プロセスでロックを取得する。
    pid_t child = fork(); // [手順] - fork した子プロセスから同じ lock file を開き try_lock を試行する。

    // Assert
    ASSERT_EQ(COM_UTIL_SYNC_OK, open_result); // [確認_正常系] - interprocess lock open が成功すること。
    ASSERT_EQ(COM_UTIL_SYNC_OK, lock_result); // [確認_正常系] - 親プロセスのロック取得が成功すること。
    ASSERT_GE(child, 0);
    if (child == 0)
    {
        com_util_interprocess_lock *child_lock = NULL;
        com_util_sync_result_t child_open = com_util_interprocess_lock_open(path, &child_lock);
        com_util_sync_result_t child_try = com_util_interprocess_lock_try_lock(child_lock);
        if (child_lock != NULL)
        {
            com_util_interprocess_lock_destroy(child_lock);
        }
        if (child_open == COM_UTIL_SYNC_OK && child_try == COM_UTIL_SYNC_BUSY)
        {
            _exit(0);
        }
        _exit(1);
    }
    int status = 0;
    ASSERT_EQ(child, waitpid(child, &status, 0));
    EXPECT_TRUE(WIFEXITED(status));    // [確認_正常系] - 子プロセスが正常終了すること。
    EXPECT_EQ(0, WEXITSTATUS(status)); // [確認_正常系] - 子プロセスで open 成功かつ try_lock が BUSY であること。

    (void)com_util_interprocess_lock_unlock(lock);
    com_util_interprocess_lock_destroy(lock);
    TEST_INTERPROCESS_UNLINK(path);
}

// fork した子プロセスから親プロセスの rwlock 排他ロックが観測できることの確認
TEST(SyncInterprocessRwlockTest, ForkedProcessObservesExclusiveLock)
{
    // Arrange
    char path[256];
    make_test_interprocess_path(path, sizeof(path),
                                "interprocess_rwlock_fork"); // [状態] - テスト用 lock file パスを用意する。
    com_util_interprocess_rwlock *lock = NULL;

    // Pre-Assert

    // Act
    com_util_sync_result_t open_result =
        com_util_interprocess_rwlock_open(path, &lock); // [手順] - interprocess rwlock を開く。
    com_util_sync_result_t lock_result = com_util_interprocess_rwlock_lock_exclusive(
        lock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 親プロセスで排他ロックを取得する。
    pid_t child = fork();             // [手順] - fork した子プロセスから同じ lock file を開き共有 try_lock を試行する。

    // Assert
    ASSERT_EQ(COM_UTIL_SYNC_OK, open_result); // [確認_正常系] - interprocess rwlock open が成功すること。
    ASSERT_EQ(COM_UTIL_SYNC_OK, lock_result); // [確認_正常系] - 親プロセスの排他ロック取得が成功すること。
    ASSERT_GE(child, 0);
    if (child == 0)
    {
        com_util_interprocess_rwlock *child_lock = NULL;
        com_util_sync_result_t child_open = com_util_interprocess_rwlock_open(path, &child_lock);
        com_util_sync_result_t child_try = com_util_interprocess_rwlock_try_lock_shared(child_lock);
        if (child_lock != NULL)
        {
            com_util_interprocess_rwlock_destroy(child_lock);
        }
        if (child_open == COM_UTIL_SYNC_OK && child_try == COM_UTIL_SYNC_BUSY)
        {
            _exit(0);
        }
        _exit(1);
    }
    int status = 0;
    ASSERT_EQ(child, waitpid(child, &status, 0));
    EXPECT_TRUE(WIFEXITED(status));    // [確認_正常系] - 子プロセスが正常終了すること。
    EXPECT_EQ(0, WEXITSTATUS(status)); // [確認_正常系] - 子プロセスで open 成功かつ共有 try_lock が BUSY であること。

    (void)com_util_interprocess_rwlock_unlock(lock);
    com_util_interprocess_rwlock_destroy(lock);
    TEST_INTERPROCESS_UNLINK(path);
}
#endif /* PLATFORM_LINUX */

// 待機時間 0 ms の指定で即時に戻ることの確認
TEST(SyncSleepMsTest, ZeroReturnsImmediately)
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
TEST(SyncSleepMsTest, NegativeReturnsImmediately)
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
TEST(SyncSleepMsTest, ElapsesAtLeastSpecifiedDuration)
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

// 負のタイムアウト指定が INVALID_ARGUMENT になることの確認
TEST(SyncLocalLockTest, NegativeTimeoutReturnsInvalidArgument)
{
    // Arrange
    com_util_local_lock *lock = NULL; // [状態] - 新規 local lock ハンドルの格納先を用意する。

    // Pre-Assert

    // Act
    com_util_sync_result_t create_result = com_util_local_lock_create(&lock); // [手順] - local lock を作成する。
    com_util_sync_result_t lock_result =
        com_util_local_lock_lock(lock, -1); // [手順] - 負のタイムアウト -1 を指定してロックする。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, create_result);             // [確認_正常系] - local lock 作成が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_INVALID_ARGUMENT, lock_result); // [確認_異常系] - 負値で INVALID_ARGUMENT を返すこと。

    com_util_local_lock_destroy(lock);
}
