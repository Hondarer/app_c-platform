#include <testfw.h>

#include <com_util/sync/sync.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

TEST(SyncRwlockTest, SharedLocksCanCoexist)
{
    // Arrange
    com_util_rwlock_t *rwlock = NULL; // [状態] - 新規 rwlock ハンドルの格納先を用意する。

    // Pre-Assert

    // Act
    com_util_sync_result_t create_result = com_util_rwlock_create(&rwlock); // [手順] - rwlock を作成する。
    com_util_sync_result_t first_lock = com_util_rwlock_lock_shared(rwlock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 1 つ目の共有ロックを取得する。
    com_util_sync_result_t second_lock = com_util_rwlock_lock_shared(rwlock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 2 つ目の共有ロックを取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, create_result); // [確認_正常系] - rwlock 作成が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, first_lock);    // [確認_正常系] - 1 つ目の共有ロック取得が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, second_lock);   // [確認_正常系] - 共有ロック同士が同時取得できること。

    (void)com_util_rwlock_unlock_shared(rwlock);
    (void)com_util_rwlock_unlock_shared(rwlock);
    com_util_rwlock_destroy(rwlock);
}

TEST(SyncRwlockTest, ExclusiveLockBlocksSharedTryLock)
{
    // Arrange
    com_util_rwlock_t *rwlock = NULL; // [状態] - 新規 rwlock ハンドルの格納先を用意する。

    // Pre-Assert

    // Act
    com_util_sync_result_t create_result = com_util_rwlock_create(&rwlock); // [手順] - rwlock を作成する。
    com_util_sync_result_t exclusive_lock = com_util_rwlock_lock_exclusive(rwlock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 排他ロックを取得する。
    com_util_sync_result_t shared_try = com_util_rwlock_try_lock_shared(rwlock); // [手順] - 排他ロック中に共有ロック取得を試行する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, create_result);    // [確認_正常系] - rwlock 作成が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, exclusive_lock);   // [確認_正常系] - 排他ロック取得が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_BUSY, shared_try);     // [確認_正常系] - 排他ロック中の共有 try lock が BUSY を返すこと。

    (void)com_util_rwlock_unlock_exclusive(rwlock);
    com_util_rwlock_destroy(rwlock);
}

TEST(SyncRwlockTest, WaitingWriterPreventsNewReader)
{
    // Arrange
    com_util_rwlock_t *rwlock = NULL; // [状態] - 新規 rwlock ハンドルの格納先を用意する。

    // Pre-Assert

    // Act
    com_util_sync_result_t create_result = com_util_rwlock_create(&rwlock); // [手順] - rwlock を作成する。
    com_util_sync_result_t shared_lock = com_util_rwlock_lock_shared(rwlock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 先行 reader を取得する。
    com_util_sync_result_t writer_try = com_util_rwlock_lock_exclusive(rwlock, 5U); // [手順] - writer 待機を発生させてタイムアウトさせる。
    com_util_sync_result_t second_reader = com_util_rwlock_try_lock_shared(rwlock); // [手順] - writer 待機後に新規 reader を試行する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, create_result);       // [確認_正常系] - rwlock 作成が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, shared_lock);         // [確認_正常系] - 先行 reader 取得が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_TIMEOUT, writer_try);     // [確認_正常系] - writer が reader 解放待ちでタイムアウトすること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, second_reader);       // [確認_正常系] - タイムアウトした writer は待機数に残らないこと。

    (void)com_util_rwlock_unlock_shared(rwlock);
    (void)com_util_rwlock_unlock_shared(rwlock);
    com_util_rwlock_destroy(rwlock);
}

TEST(SyncAppLockTest, DescriptorRoundTripReopensSameLock)
{
    // Arrange
    char path[256];
    snprintf(path, sizeof(path), "/tmp/com_util_app_lock_%ld.lock", (long)getpid()); // [状態] - テスト用 lock file パスを用意する。
    com_util_app_lock_t *lock = NULL;
    com_util_app_lock_t *restored = NULL;
    unsigned char descriptor[512];
    size_t descriptor_size = sizeof(descriptor);

    // Pre-Assert

    // Act
    com_util_sync_result_t create_result = com_util_app_lock_create(path, &lock); // [手順] - app lock を作成する。
    com_util_sync_result_t export_result = com_util_app_lock_export_descriptor(lock, descriptor, &descriptor_size); // [手順] - descriptor をバッファへ出力する。
    com_util_sync_result_t import_result = com_util_app_lock_import_descriptor(descriptor, descriptor_size, &restored); // [手順] - descriptor から app lock を復元する。
    com_util_sync_result_t exclusive_lock = com_util_app_lock_lock_exclusive(lock, COM_UTIL_SYNC_NO_WAIT); // [手順] - 元ハンドルで排他ロックを取得する。
    com_util_sync_result_t shared_try = com_util_app_lock_try_lock_shared(restored); // [手順] - 復元ハンドルで共有ロック取得を試行する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, create_result);       // [確認_正常系] - app lock 作成が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, export_result);       // [確認_正常系] - descriptor 出力が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, import_result);       // [確認_正常系] - descriptor から復元できること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, exclusive_lock);      // [確認_正常系] - 元ハンドルで排他ロックを取得できること。
    EXPECT_EQ(COM_UTIL_SYNC_BUSY, shared_try);        // [確認_正常系] - 復元ハンドルが同じ排他インスタンスを参照すること。

    (void)com_util_app_lock_unlock(lock);
    com_util_app_lock_destroy(restored);
    com_util_app_lock_destroy(lock);
    unlink(path);
}

TEST(SyncAppLockTest, ExportReportsRequiredDescriptorSize)
{
    // Arrange
    char path[256];
    snprintf(path, sizeof(path), "/tmp/com_util_app_lock_size_%ld.lock", (long)getpid()); // [状態] - テスト用 lock file パスを用意する。
    com_util_app_lock_t *lock = NULL;
    size_t descriptor_size = 0U;

    // Pre-Assert

    // Act
    com_util_sync_result_t create_result = com_util_app_lock_create(path, &lock); // [手順] - app lock を作成する。
    com_util_sync_result_t export_result = com_util_app_lock_export_descriptor(lock, NULL, &descriptor_size); // [手順] - NULL バッファで必要サイズを問い合わせる。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, create_result);                    // [確認_正常系] - app lock 作成が成功すること。
    EXPECT_EQ(COM_UTIL_SYNC_BUFFER_TOO_SMALL, export_result);      // [確認_正常系] - バッファ不足が通知されること。
    EXPECT_GT(descriptor_size, 20U);                               // [確認_正常系] - descriptor に必要なサイズが返ること。

    com_util_app_lock_destroy(lock);
    unlink(path);
}

TEST(SyncAppLockTest, CorruptDescriptorIsRejected)
{
    // Arrange
    unsigned char descriptor[20] = {0}; // [状態] - magic を持たない descriptor を用意する。
    com_util_app_lock_t *lock = NULL;

    // Pre-Assert

    // Act
    com_util_sync_result_t result = com_util_app_lock_import_descriptor(descriptor, sizeof(descriptor), &lock); // [手順] - 不正 descriptor を import する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_CORRUPT_DESCRIPTOR, result); // [確認_異常系] - 不正 descriptor が拒否されること。
    EXPECT_EQ(NULL, lock);                               // [確認_異常系] - ハンドルが生成されないこと。
}
