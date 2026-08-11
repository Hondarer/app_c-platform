#include <testfw.h>
#include <mock_pthread.h>
#include <mock_stdlib.h>
#include <sys/mock_mman.h>
#include <mock_com_util.h>
#include <com_util/runtime/memory_lock.h>

#include <cstdlib>
#include <cerrno>
#include <thread>

using testing::_;
using testing::DoAll;
using testing::NiceMock;
using testing::Return;

class memoryLockTest : public Test
{
};

struct self_lock_thread_result
{
    com_util_memory_lock_scope *scope;
    int result;
    int pad;
};

static void lock_self_current_in_thread(self_lock_thread_result *thread_result)
{
    com_util_memory_lock_self_options options = {};

    thread_result->scope = nullptr;
    options.flags = COM_UTIL_MEMORY_LOCK_CURRENT;
    options.stack_prefault_bytes = 4096U;
    thread_result->result = com_util_memory_lock_self(&options, &thread_result->scope);
}

static void release_self_scope_in_thread(com_util_memory_lock_scope *scope, int *result)
{
    *result = com_util_memory_lock_scope_release(scope);
}

// range ロック API が不正引数を検出することの確認
TEST_F(memoryLockTest, test_range_rejects_invalid_arguments)
{
    // Arrange
    unsigned char buffer[16] = {}; // [状態] - 16 バイトのバッファーを用意する。

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_memory_lock_range(
                  NULL, sizeof(buffer))); // [確認_異常系] - lock_range (addr NULL) が INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        com_util_memory_lock_range(buffer, 0U)); // [確認_異常系] - lock_range (size 0) が INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_memory_unlock_range(
                  NULL, sizeof(buffer))); // [確認_異常系] - unlock_range (addr NULL) が INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_memory_unlock_range(
                  buffer, 0U)); // [確認_異常系] - unlock_range (size 0) が INVALID_ARGUMENT を返すこと。
}

// ヒープ バッファーの range ロックと解除が成功することの確認
TEST_F(memoryLockTest, test_range_locks_and_unlocks_heap_buffer)
{
    // Arrange
    void *buffer = malloc(4096U); // [状態] - 4096 バイトのヒープ バッファーを確保する。
    ASSERT_NE(nullptr, buffer);

    // Pre-Assert

    // Act
    int lock_result =
        com_util_memory_lock_range(buffer, 4096U); // [手順] - バッファーを com_util_memory_lock_range でロックする。
    int unlock_result = COM_UTIL_ERR_UNKNOWN;
    if (lock_result == COM_UTIL_OK)
    {
        unlock_result = com_util_memory_unlock_range(
            buffer, 4096U); // [手順] - ロック成功時に com_util_memory_unlock_range で解除する。
    }

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              lock_result); // [確認_正常系] - com_util_memory_lock_range の戻り値が OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              unlock_result); // [確認_正常系] - com_util_memory_unlock_range の戻り値が OK であること。

    // Cleanup
    free(buffer);
}

#if defined(PLATFORM_LINUX)
// mlock と munlock の OS エラーがメモリ ロック結果へ変換されることの確認
TEST_F(memoryLockTest, test_range_maps_mlock_errors)
{
    // Arrange
    NiceMock<Mock_sys_mman> mock_mman;
    unsigned char buffer[32] = {};
    EXPECT_CALL(mock_mman, mlock(_, _, _, _, _)).WillOnce(Return(-1));
    EXPECT_CALL(mock_mman, munlock(_, _, _, _, _)).WillOnce(Return(-1));
    errno = ENOMEM;

    // Pre-Assert

    // Act
    int lock_result = com_util_memory_lock_range(
        buffer, sizeof(buffer)); // [手順] - mlock が ENOMEM で失敗する状態で範囲をロックする。
    errno = EACCES;
    int unlock_result = com_util_memory_unlock_range(
        buffer, sizeof(buffer)); // [手順] - munlock が EACCES で失敗する状態で範囲を解除する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_LIMIT_EXCEEDED,
              lock_result); // [確認_異常系] - mlock の ENOMEM が LIMIT_EXCEEDED へ変換されること。
    EXPECT_EQ(COM_UTIL_ERR_PERMISSION_DENIED,
              unlock_result); // [確認_異常系] - munlock の EACCES が PERMISSION_DENIED へ変換されること。
}

// mlockall の失敗が self lock の生成失敗として通知されることの確認
TEST_F(memoryLockTest, test_lock_self_reports_mlockall_failure)
{
    // Arrange
    NiceMock<Mock_sys_mman> mock_mman;
    NiceMock<Mock_com_util> mock_com_util;
    com_util_memory_lock_self_options options = {};
    com_util_memory_lock_scope *scope = nullptr;
    options.flags = COM_UTIL_MEMORY_LOCK_CURRENT;
    EXPECT_CALL(mock_mman, mlockall(_, _, _, _))
        .WillOnce(testing::Invoke(
            [](const char *, const int, const char *, const int)
            {
                errno = ENOMEM;
                return -1;
            }));
    EXPECT_CALL(mock_com_util, com_util_call_once(_, _))
        .WillOnce(testing::Invoke(
            [](com_util_once_flag *flag, com_util_once_fn func)
            {
                func();
                flag->state = 2;
            }));
    EXPECT_CALL(mock_com_util, com_util_local_lock_create(_))
        .WillOnce(testing::Invoke(delegate_real_com_util_local_lock_create));
    EXPECT_CALL(mock_com_util, com_util_local_lock_lock(_, COM_UTIL_SYNC_WAIT_FOREVER)).WillOnce(Return(COM_UTIL_OK));
    EXPECT_CALL(mock_com_util, com_util_local_lock_unlock(_)).WillOnce(Return(COM_UTIL_OK));

    // Pre-Assert

    // Act
    int result = com_util_memory_lock_self(
        &options, &scope); // [手順] - mlockall が ENOMEM で失敗する状態で self lock を生成する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_LIMIT_EXCEEDED,
              result);         // [確認_異常系] - mlockall の ENOMEM が LIMIT_EXCEEDED へ変換されること。
    EXPECT_EQ(nullptr, scope); // [確認_異常系] - self lock の scope が生成されないこと。
}

// self lock の解除が munlockall のエラーを返すことの確認
TEST_F(memoryLockTest, test_scope_release_reports_munlockall_failure)
{
    // Arrange
    NiceMock<Mock_sys_mman> mock_mman;
    com_util_memory_lock_self_options options = {};
    com_util_memory_lock_scope *scope = nullptr;
    options.flags = COM_UTIL_MEMORY_LOCK_CURRENT;
    EXPECT_CALL(mock_mman, mlockall(_, _, _, _)).WillOnce(Return(0));
    EXPECT_CALL(mock_mman, munlockall(_, _, _)).WillOnce(Return(-1));

    // Pre-Assert
    ASSERT_EQ(COM_UTIL_OK, com_util_memory_lock_self(&options, &scope));
    ASSERT_NE(nullptr, scope);
    errno = EACCES;

    // Act
    int result =
        com_util_memory_lock_scope_release(scope); // [手順] - munlockall が EACCES で失敗する状態で scope を解放する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_PERMISSION_DENIED,
              result); // [確認_異常系] - munlockall の EACCES が PERMISSION_DENIED へ変換されること。
}

// stack prefault の pthread_getattr_np 失敗が self lock の生成失敗になることの確認
TEST_F(memoryLockTest, test_lock_self_reports_stack_attribute_failure)
{
    // Arrange
    NiceMock<Mock_pthread> mock_pthread;
    com_util_memory_lock_self_options options = {};
    com_util_memory_lock_scope *scope = nullptr;
    options.flags = COM_UTIL_MEMORY_LOCK_CURRENT;
    options.stack_prefault_bytes = 1U;
    EXPECT_CALL(mock_pthread, pthread_getattr_np(_, _, _, _, _)).WillOnce(Return(EINVAL));

    // Pre-Assert

    // Act
    int result =
        com_util_memory_lock_self(&options, &scope); // [手順] - スタック属性取得が失敗する状態で self lock を生成する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              result);         // [確認_異常系] - スタック属性取得失敗が UNKNOWN になること。
    EXPECT_EQ(nullptr, scope); // [確認_異常系] - stack prefault 失敗時に scope が生成されないこと。
}

// stack の取得範囲が安全余白を満たさない場合に制限超過を返すことの確認
TEST_F(memoryLockTest, test_lock_self_rejects_insufficient_stack_range)
{
    // Arrange
    NiceMock<Mock_pthread> mock_pthread;
    com_util_memory_lock_self_options options = {};
    com_util_memory_lock_scope *scope = nullptr;
    options.flags = COM_UTIL_MEMORY_LOCK_CURRENT;
    options.stack_prefault_bytes = 1U;
    EXPECT_CALL(mock_pthread, pthread_getattr_np(_, _, _, _, _)).WillOnce(Return(0));
    EXPECT_CALL(mock_pthread, pthread_attr_getstack(_, _, _, _, _, _))
        .WillOnce(DoAll(
            testing::Invoke(
                [](const char *, const int, const char *, const pthread_attr_t *, void **stack_addr, size_t *stack_size)
                {
                    *stack_addr = nullptr;
                    *stack_size = 1U;
                }),
            Return(0)));
    EXPECT_CALL(mock_pthread, pthread_attr_destroy(_, _, _, _)).WillOnce(Return(0));

    // Pre-Assert

    // Act
    int result = com_util_memory_lock_self(
        &options, &scope); // [手順] - スタック安全余白を満たさない状態で self lock を生成する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_LIMIT_EXCEEDED,
              result);         // [確認_異常系] - スタック不足が LIMIT_EXCEEDED になること。
    EXPECT_EQ(nullptr, scope); // [確認_異常系] - スタック不足時に scope が生成されないこと。
}
#endif /* PLATFORM_LINUX */

// self ロック API が不正引数を検出することの確認
TEST_F(memoryLockTest, test_lock_self_rejects_invalid_arguments)
{
    // Arrange
    com_util_memory_lock_scope *scope = nullptr;
    com_util_memory_lock_self_options options = {};
    options.flags =
        COM_UTIL_MEMORY_LOCK_CURRENT; // [状態] - flags を COM_UTIL_MEMORY_LOCK_CURRENT としたオプションを用意する。

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_memory_lock_self(NULL, &scope)); // [確認_異常系] - options NULL が INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        com_util_memory_lock_self(&options, NULL)); // [確認_異常系] - scope_out NULL が INVALID_ARGUMENT を返すこと。
    options.flags = 0;
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_memory_lock_self(&options, &scope)); // [確認_異常系] - flags 0 が INVALID_ARGUMENT を返すこと。
    options.flags = COM_UTIL_MEMORY_LOCK_CURRENT | 0x100;
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_memory_lock_self(&options,
                                        &scope)); // [確認_異常系] - 未定義フラグ 0x100 が INVALID_ARGUMENT を返すこと。
}

// scope 解放が NULL を受理することの確認
TEST_F(memoryLockTest, test_scope_release_accepts_null)
{
    // Arrange

    // Pre-Assert

    // Act
    int rtc_memory_lock = com_util_memory_lock_scope_release(
        NULL); // [手順] - NULL scope で com_util_memory_lock_scope_release を呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        rtc_memory_lock); // [確認_正常系] - com_util_memory_lock_scope_release の戻り値が COM_UTIL_OK であること。
}

// 過大な stack_prefault_bytes が拒否されることの確認
TEST_F(memoryLockTest, test_lock_self_rejects_excessive_stack_prefault)
{
    // Arrange
    com_util_memory_lock_scope *scope = nullptr;
    com_util_memory_lock_self_options options = {};
    options.flags = COM_UTIL_MEMORY_LOCK_CURRENT;
    options.stack_prefault_bytes = static_cast<size_t>(-1); // [状態] - stack_prefault_bytes を過大な SIZE_MAX とする。

    // Pre-Assert

    // Act
    int rtc_memory_lock =
        com_util_memory_lock_self(&options, &scope); // [手順] - 過大な設定で com_util_memory_lock_self を呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_LIMIT_EXCEEDED,
        rtc_memory_lock); // [確認_異常系] - com_util_memory_lock_self の戻り値が COM_UTIL_ERR_LIMIT_EXCEEDED であること。
    EXPECT_EQ(nullptr, scope); // [確認_異常系] - scope が NULL のままであること。
}

// 複数スレッドから独立した self ロック scope を取得できることの確認
TEST_F(memoryLockTest, test_lock_self_allows_independent_scopes_from_multiple_threads)
{
    // Arrange
    self_lock_thread_result first = {};
    self_lock_thread_result second = {}; // [状態] - 2 スレッド分の結果格納先を用意する。

    // Pre-Assert

    // Act
    std::thread first_thread(lock_self_current_in_thread, &first);
    std::thread second_thread(lock_self_current_in_thread,
                              &second); // [手順] - 2 つのスレッドから com_util_memory_lock_self を呼び出す。
    first_thread.join();
    second_thread.join();

    // Assert
    if ((first.result != COM_UTIL_OK) || (second.result != COM_UTIL_OK))
    {

        // Cleanup
        (void)com_util_memory_lock_scope_release(first.scope);
        (void)com_util_memory_lock_scope_release(second.scope);
        GTEST_SKIP() << "self memory lock is not available in this environment";
    }

    ASSERT_NE(nullptr, first.scope);  // [確認_正常系] - 1 つ目のスレッドで scope が取得できること。
    ASSERT_NE(nullptr, second.scope); // [確認_正常系] - 2 つ目のスレッドで scope が取得できること。

    int first_release = COM_UTIL_ERR_UNKNOWN;
    int second_release = COM_UTIL_ERR_UNKNOWN;
    std::thread first_release_thread(release_self_scope_in_thread, first.scope, &first_release);
    std::thread second_release_thread(release_self_scope_in_thread, second.scope,
                                      &second_release); // [手順] - 別スレッドから各 scope を解放する。
    first_release_thread.join();
    second_release_thread.join();

    EXPECT_EQ(COM_UTIL_OK, first_release);  // [確認_正常系] - 1 つ目の scope の解放が OK であること。
    EXPECT_EQ(COM_UTIL_OK, second_release); // [確認_正常系] - 2 つ目の scope の解放が OK であること。
}

#if defined(PLATFORM_WINDOWS)
// Windows で未サポートのフラグが拒否されることの確認
TEST_F(memoryLockTest, test_lock_self_rejects_unsupported_flags_on_windows)
{
    // Arrange
    com_util_memory_lock_scope *scope = nullptr;
    com_util_memory_lock_self_options options = {}; // [状態] - 空のオプションを用意する。

    // Pre-Assert

    // Act
    // Assert
    options.flags = COM_UTIL_MEMORY_LOCK_FUTURE;
    EXPECT_EQ(COM_UTIL_ERR_UNSUPPORTED,
              com_util_memory_lock_self(&options, &scope)); // [確認_異常系] - FUTURE フラグが UNSUPPORTED になること。
    options.flags = COM_UTIL_MEMORY_LOCK_ONFAULT;
    EXPECT_EQ(COM_UTIL_ERR_UNSUPPORTED,
              com_util_memory_lock_self(&options, &scope)); // [確認_異常系] - ONFAULT フラグが UNSUPPORTED になること。
    options.flags = COM_UTIL_MEMORY_LOCK_CURRENT | COM_UTIL_MEMORY_LOCK_FUTURE;
    EXPECT_EQ(COM_UTIL_ERR_UNSUPPORTED,
              com_util_memory_lock_self(&options,
                                        &scope)); // [確認_異常系] - CURRENT と FUTURE の併用が UNSUPPORTED になること。
}
#endif
