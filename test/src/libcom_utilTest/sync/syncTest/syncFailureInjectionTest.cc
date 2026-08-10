#include <testfw.h>

#include "syncTestHelper.h"

#if defined(PLATFORM_LINUX)

    #include <mock_pthread.h>
    #include <mock_stdlib.h>
    #include <mock_string.h>
    #include <sys/mock_file.h>

using testing::_;
using testing::DoDefault;
using testing::NiceMock;
using testing::Return;

class syncFailureInjectionTest : public Test
{
  protected:
    char path_[256];

    void SetUp() override
    {
        make_test_interprocess_path(path_, sizeof(path_), "failure_injection");
    }

    void TearDown() override
    {
        TEST_INTERPROCESS_UNLINK(path_);
    }
};

/*
 * com_util_local_lock_create
 */

// ミューテックスの初期化に失敗した場合に生成が失敗することの確認
// Windows は CRITICAL_SECTION を使うため、この失敗経路は Linux のみに存在する
TEST_F(syncFailureInjectionTest, local_lock_create_fails_when_mutex_init_fails)
{
    // Arrange
    NiceMock<Mock_pthread> mock_pthread;
    com_util_local_lock *lock = NULL; // [状態] - ハンドルの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_pthread, pthread_mutex_init(_, _, _, _, _))
        .WillOnce(Return(ENOMEM))
        .WillRepeatedly(
            DoDefault()); // [Pre-Assert確認_異常系] - pthread_mutex_init が 1 回目に呼び出されること。
                          // [Pre-Assert手順] - 1 回目は ENOMEM を返却し、以降は本物へ委譲する。

    // Act
    int rtc = com_util_local_lock_create(&lock); // [手順] - com_util_local_lock_create を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc); // [確認_異常系] - com_util_local_lock_create の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ((com_util_local_lock *)NULL, lock); // [確認_異常系] - ハンドルが設定されないこと。
}

// ハンドルの確保に失敗した場合に生成が失敗することの確認
TEST_F(syncFailureInjectionTest, local_lock_create_fails_when_allocation_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    com_util_local_lock *lock = NULL; // [状態] - ハンドルの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, calloc(_, _, _, _, _))
        .WillOnce(Return(nullptr))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - calloc が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - 1 回目は NULL を返却し、以降は本物へ委譲する。

    // Act
    int rtc = com_util_local_lock_create(&lock); // [手順] - com_util_local_lock_create を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc); // [確認_異常系] - com_util_local_lock_create の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ((com_util_local_lock *)NULL, lock); // [確認_異常系] - ハンドルが設定されないこと。
}

/*
 * com_util_interprocess_lock_open
 */

// ロック ファイルを開けない場合に取得が失敗することの確認
TEST_F(syncFailureInjectionTest, interprocess_lock_open_fails_for_unopenable_path)
{
    // Arrange
    com_util_interprocess_lock *lock = NULL; // [状態] - ハンドルの格納先を用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_interprocess_lock_open("/proc/com_util_unopenable_for_test/lock",
                                              &lock); // [手順] - 作成できないパスを指定して呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc); // [確認_異常系] - com_util_interprocess_lock_open の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ((com_util_interprocess_lock *)NULL, lock); // [確認_異常系] - ハンドルが設定されないこと。
}

// 識別子の複製に失敗した場合に取得が失敗することの確認
// Windows は識別子を CreateMutexW へ直接渡すため、この失敗経路は Linux のみに存在する
TEST_F(syncFailureInjectionTest, interprocess_lock_open_fails_when_identity_duplication_fails)
{
    // Arrange
    NiceMock<Mock_string> mock_string;
    com_util_interprocess_lock *lock = NULL; // [状態] - ハンドルの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_string, strdup(_, _, _, _))
        .WillOnce(Return(nullptr))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - strdup が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - 1 回目は NULL を返却し、以降は本物へ委譲する。

    // Act
    int rtc = com_util_interprocess_lock_open(path_, &lock); // [手順] - com_util_interprocess_lock_open を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc); // [確認_異常系] - com_util_interprocess_lock_open の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ((com_util_interprocess_lock *)NULL, lock); // [確認_異常系] - ハンドルが設定されないこと。
}

// ハンドルの確保に失敗した場合に取得が失敗することの確認
TEST_F(syncFailureInjectionTest, interprocess_lock_open_fails_when_allocation_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    com_util_interprocess_lock *lock = NULL; // [状態] - ハンドルの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, calloc(_, _, _, _, _))
        .WillOnce(Return(nullptr))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - calloc が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - 1 回目は NULL を返却し、以降は本物へ委譲する。

    // Act
    int rtc = com_util_interprocess_lock_open(path_, &lock); // [手順] - com_util_interprocess_lock_open を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc); // [確認_異常系] - com_util_interprocess_lock_open の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ((com_util_interprocess_lock *)NULL, lock); // [確認_異常系] - ハンドルが設定されないこと。
}

/*
 * flock の失敗
 */

// ロック解除に失敗した場合に通知されることの確認
// Windows は ReleaseMutex を使うため、この失敗経路は Linux のみに存在する
TEST_F(syncFailureInjectionTest, interprocess_lock_unlock_reports_flock_failure)
{
    // Arrange
    NiceMock<Mock_sys_file> mock_sys_file;
    com_util_interprocess_lock *lock = NULL;

    ASSERT_EQ(COM_UTIL_OK, com_util_interprocess_lock_open(path_, &lock));
    ASSERT_EQ(COM_UTIL_OK,
              com_util_interprocess_lock_lock(lock, COM_UTIL_SYNC_NO_WAIT)); // [状態] - ロックを取得済みにする。

    // Pre-Assert
    EXPECT_CALL(mock_sys_file, flock(_, _, _, _, LOCK_UN))
        .WillOnce(Return(-1))
        .WillRepeatedly(
            DoDefault()); // [Pre-Assert確認_異常系] - flock が LOCK_UN を指定して 1 回目に呼び出されること。
                          // [Pre-Assert手順] - 1 回目は -1 を返却し、以降は本物へ委譲する。

    // Act
    int rtc = com_util_interprocess_lock_unlock(lock); // [手順] - com_util_interprocess_lock_unlock を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc); // [確認_異常系] - com_util_interprocess_lock_unlock の戻り値が COM_UTIL_ERR_UNKNOWN であること。

    // Cleanup
    com_util_interprocess_lock_destroy(lock);
}

/*
 * com_util_thread_create
 */

// スレッドの生成に失敗した場合に通知されることの確認
// Windows は _beginthreadex を使うため、この失敗経路は Linux のみに存在する
TEST_F(syncFailureInjectionTest, thread_create_fails_when_pthread_create_fails)
{
    // Arrange
    NiceMock<Mock_pthread> mock_pthread;
    com_util_thread *thread = NULL; // [状態] - ハンドルの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_pthread, pthread_create(_, _, _, _, _, _, _))
        .WillOnce(Return(EAGAIN))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - pthread_create が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - 1 回目は EAGAIN を返却し、以降は本物へ委譲する。

    // Act
    int rtc = com_util_thread_create(&thread, [](void *) {}, NULL); // [手順] - com_util_thread_create を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc); // [確認_異常系] - com_util_thread_create の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ((com_util_thread *)NULL, thread); // [確認_異常系] - ハンドルが設定されないこと。
}

#endif /* PLATFORM_LINUX */
