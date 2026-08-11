#include <testfw.h>

#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)

    #include <mock_pthread.h>
    #include <mock_stdlib.h>
    #include <sys/mock_file.h>

    #include <com_util/sync/sync.h>

    #include <errno.h>
    #include <fcntl.h>
    #include <string.h>

    #include "syncTestHelper.h"

using testing::_;
using testing::DoDefault;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;

// local lock の trylock で未知の pthread エラーを分類することの確認
TEST(syncAdditionalFailureTest, local_lock_maps_unknown_trylock_error)
{
    // Arrange
    com_util_local_lock *lock = NULL;
    ASSERT_EQ(COM_UTIL_OK, com_util_local_lock_create(&lock));
    NiceMock<Mock_pthread> mock_pthread;
    EXPECT_CALL(mock_pthread, pthread_mutex_trylock(_, _, _, _))
        .WillOnce(Return(EINVAL)); // [Pre-Assert確認_異常系] - pthread_mutex_trylock が EINVAL を返すこと。

    // Pre-Assert

    // Act
    int result = com_util_local_lock_try_lock(lock); // [手順] - EINVAL を返す trylock を実行する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              result); // [確認_異常系] - com_util_local_lock_try_lock の戻り値が COM_UTIL_ERR_UNKNOWN であること。

    // Cleanup
    com_util_local_lock_destroy(lock);
}

// condvar の待機・通知エラーを共通結果へ変換することの確認
TEST(syncAdditionalFailureTest, condvar_maps_wait_signal_and_broadcast_errors)
{
    // Arrange
    com_util_condvar *cv = NULL;
    com_util_local_lock *lock = NULL;
    ASSERT_EQ(COM_UTIL_OK, com_util_condvar_create(&cv));
    ASSERT_EQ(COM_UTIL_OK, com_util_local_lock_create(&lock));
    NiceMock<Mock_pthread> mock_pthread;
    EXPECT_CALL(mock_pthread, pthread_cond_wait(_, _, _, _, _)).WillOnce(Return(EINVAL));
    EXPECT_CALL(mock_pthread, pthread_cond_timedwait(_, _, _, _, _, _)).WillOnce(Return(ETIMEDOUT));
    EXPECT_CALL(mock_pthread, pthread_cond_signal(_, _, _, _)).WillOnce(Return(EINVAL));
    EXPECT_CALL(mock_pthread, pthread_cond_broadcast(_, _, _, _)).WillOnce(Return(EINVAL));

    // Pre-Assert

    // Act
    int wait_result =
        com_util_condvar_wait(cv, lock, COM_UTIL_SYNC_WAIT_FOREVER); // [手順] - cond_wait の EINVAL 失敗を注入する。
    int timed_wait_result =
        com_util_condvar_wait(cv, lock, COM_UTIL_SYNC_NO_WAIT); // [手順] - cond_timedwait の timeout を注入する。
    int signal_result = com_util_condvar_signal(cv);            // [手順] - cond_signal の EINVAL 失敗を注入する。
    int broadcast_result = com_util_condvar_broadcast(cv);      // [手順] - cond_broadcast の EINVAL 失敗を注入する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, wait_result); // [確認_異常系] - WAIT_FOREVER の condvar 待機が UNKNOWN になること。
    EXPECT_EQ(COM_UTIL_ERR_TIMEOUT,
              timed_wait_result);                      // [確認_正常系] - NO_WAIT の condvar 待機が TIMEOUT になること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, signal_result);    // [確認_異常系] - condvar signal が UNKNOWN になること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, broadcast_result); // [確認_異常系] - condvar broadcast が UNKNOWN になること。

    // Cleanup
    com_util_condvar_destroy(cv);
    com_util_local_lock_destroy(lock);
}

// condvar 属性の時計設定失敗を生成失敗として処理することの確認
TEST(syncAdditionalFailureTest, condvar_create_reports_clock_attribute_failure)
{
    // Arrange
    NiceMock<Mock_pthread> mock_pthread;
    com_util_condvar *cv = NULL;
    EXPECT_CALL(mock_pthread, pthread_condattr_setclock(_, _, _, _, CLOCK_MONOTONIC))
        .WillOnce(Return(EINVAL)); // [Pre-Assert確認_異常系] - condvar の時計属性設定が失敗すること。

    // Pre-Assert

    // Act
    int result = com_util_condvar_create(&cv); // [手順] - 時計属性設定失敗を注入して condvar を生成する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              result); // [確認_異常系] - com_util_condvar_create の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ((com_util_condvar *)NULL, cv); // [確認_異常系] - 生成失敗時に condvar が NULL であること。
}

// condvar 初期化失敗を生成失敗として処理することの確認
TEST(syncAdditionalFailureTest, condvar_create_reports_native_initialization_failure)
{
    // Arrange
    NiceMock<Mock_pthread> mock_pthread;
    com_util_condvar *cv = NULL;
    EXPECT_CALL(mock_pthread, pthread_cond_init(_, _, _, _, _))
        .WillOnce(Return(EINVAL)); // [Pre-Assert確認_異常系] - native condvar 初期化が失敗すること。

    // Pre-Assert

    // Act
    int result = com_util_condvar_create(&cv); // [手順] - native condvar 初期化失敗を注入して生成する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              result);                       // [確認_異常系] - 初期化失敗時の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ((com_util_condvar *)NULL, cv); // [確認_異常系] - 初期化失敗時に condvar が NULL であること。
}

// rwlock の未取得状態での解放要求を拒否することの確認
TEST(syncAdditionalFailureTest, rwlock_rejects_unlock_without_ownership)
{
    // Arrange
    com_util_local_rwlock *rwlock = NULL;
    ASSERT_EQ(COM_UTIL_OK, com_util_local_rwlock_create(&rwlock));

    // Pre-Assert

    // Act
    int shared_result =
        com_util_local_rwlock_unlock_shared(rwlock); // [手順] - 共有ロック未取得で unlock_shared を呼び出す。
    int exclusive_result =
        com_util_local_rwlock_unlock_exclusive(rwlock); // [手順] - 排他ロック未取得で unlock_exclusive を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              shared_result); // [確認_異常系] - 未取得の共有ロック解放が INVALID_ARGUMENT になること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              exclusive_result); // [確認_異常系] - 未取得の排他ロック解放が INVALID_ARGUMENT になること。

    // Cleanup
    com_util_local_rwlock_destroy(rwlock);
}

// thread join の pthread エラーを未知エラーへ変換することの確認
TEST(syncAdditionalFailureTest, thread_join_reports_pthread_failure)
{
    // Arrange
    com_util_thread *thread = NULL;
    ASSERT_EQ(COM_UTIL_OK, com_util_thread_create(&thread, [](void *) {}, NULL));
    NiceMock<Mock_pthread> mock_pthread;
    EXPECT_CALL(mock_pthread, pthread_join(_, _, _, _, _)).WillOnce(Return(EINVAL));
    EXPECT_CALL(mock_pthread, pthread_detach(_, _, _, _)).WillOnce(DoDefault());

    // Pre-Assert

    // Act
    int result =
        com_util_thread_join(thread, COM_UTIL_SYNC_WAIT_FOREVER); // [手順] - pthread_join の EINVAL 失敗を注入する。
    com_util_thread_detach(thread); // [手順] - join 失敗後の thread を detach して解放する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              result); // [確認_異常系] - com_util_thread_join の戻り値が COM_UTIL_ERR_UNKNOWN であること。
}

// thread の即時 join timeout を TIMEOUT へ変換することの確認
TEST(syncAdditionalFailureTest, thread_join_no_wait_reports_timeout)
{
    // Arrange
    com_util_thread *thread = NULL;
    ASSERT_EQ(COM_UTIL_OK, com_util_thread_create(&thread, [](void *) {}, NULL));
    NiceMock<Mock_pthread> mock_pthread;
    EXPECT_CALL(mock_pthread, pthread_tryjoin_np(_, _, _, _, _)).WillOnce(Return(EBUSY));
    EXPECT_CALL(mock_pthread, pthread_detach(_, _, _, _)).WillOnce(DoDefault());

    // Pre-Assert

    // Act
    int result =
        com_util_thread_join(thread, COM_UTIL_SYNC_NO_WAIT); // [手順] - pthread_tryjoin_np の EBUSY を注入する。
    com_util_thread_detach(thread);                          // [手順] - timeout 後の thread を detach して解放する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_TIMEOUT,
              result); // [確認_正常系] - com_util_thread_join の戻り値が COM_UTIL_ERR_TIMEOUT であること。
}

// interprocess lock の EWOULDBLOCK と EINTR を処理することの確認
TEST(syncAdditionalFailureTest, interprocess_lock_maps_busy_and_retries_eintr)
{
    // Arrange
    char path[256];
    make_test_interprocess_path(path, sizeof(path), "additional_failure");
    com_util_interprocess_lock *lock = NULL;
    ASSERT_EQ(COM_UTIL_OK, com_util_interprocess_lock_open(path, &lock));
    NiceMock<Mock_sys_file> mock_sys_file;
    EXPECT_CALL(mock_sys_file, flock(_, _, _, _, LOCK_EX | LOCK_NB))
        .WillOnce(Invoke(
            [](const char *, const int, const char *, const int, const int)
            {
                errno = EWOULDBLOCK;
                return -1;
            }));

    // Pre-Assert

    // Act
    int busy_result = com_util_interprocess_lock_try_lock(
        lock); // [手順] - 非ブロッキング flock が EWOULDBLOCK になる状態で取得する。

    EXPECT_CALL(mock_sys_file, flock(_, _, _, _, LOCK_EX))
        .WillOnce(Invoke(
            [](const char *, const int, const char *, const int, const int)
            {
                errno = EINTR;
                return -1;
            }))
        .WillOnce(Return(0));
    int retry_result = com_util_interprocess_lock_lock(
        lock, COM_UTIL_SYNC_WAIT_FOREVER); // [手順] - EINTR 後に成功する flock を実行する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUSY,
              busy_result); // [確認_正常系] - try_lock が BUSY を返すこと。
    EXPECT_EQ(COM_UTIL_OK,
              retry_result); // [確認_正常系] - EINTR 後の interprocess lock 取得が COM_UTIL_OK であること。

    // Cleanup
    EXPECT_CALL(mock_sys_file, flock(_, _, _, _, LOCK_UN)).WillOnce(Return(0));
    (void)com_util_interprocess_lock_unlock(lock);
    com_util_interprocess_lock_destroy(lock);
    TEST_INTERPROCESS_UNLINK(path);
}

// thread ハンドル確保の malloc 失敗を未知エラーへ変換することの確認
TEST(syncAdditionalFailureTest, thread_create_reports_context_allocation_failure)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    com_util_thread *thread = NULL;
    EXPECT_CALL(mock_stdlib, calloc(_, _, _, _, _)).WillOnce(Return(nullptr));

    // Pre-Assert

    // Act
    int result =
        com_util_thread_create(&thread, [](void *) {}, NULL); // [手順] - thread ハンドルの calloc 失敗を注入する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              result); // [確認_異常系] - com_util_thread_create の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ((com_util_thread *)NULL, thread); // [確認_異常系] - 生成失敗時に thread が NULL であること。
}

#endif /* PLATFORM_LINUX */
