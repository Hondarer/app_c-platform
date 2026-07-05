#include <testfw.h>
#include <com_util/runtime/memory_lock.h>

#include <cstdlib>
#include <thread>

class memoryLockTest : public Test
{
};

struct self_lock_thread_result
{
    com_util_memory_lock_scope *scope;
    com_util_memory_lock_result_t result;
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

static void release_self_scope_in_thread(com_util_memory_lock_scope *scope, com_util_memory_lock_result_t *result)
{
    *result = com_util_memory_lock_scope_release(scope);
}

TEST_F(memoryLockTest, test_range_rejects_invalid_arguments)
{
    // Arrange
    unsigned char buffer[16] = {};

    // Act & Assert
    EXPECT_EQ(COM_UTIL_MEMORY_LOCK_INVALID_ARGUMENT, com_util_memory_lock_range(NULL, sizeof(buffer)));
    EXPECT_EQ(COM_UTIL_MEMORY_LOCK_INVALID_ARGUMENT, com_util_memory_lock_range(buffer, 0U));
    EXPECT_EQ(COM_UTIL_MEMORY_LOCK_INVALID_ARGUMENT, com_util_memory_unlock_range(NULL, sizeof(buffer)));
    EXPECT_EQ(COM_UTIL_MEMORY_LOCK_INVALID_ARGUMENT, com_util_memory_unlock_range(buffer, 0U));
}

TEST_F(memoryLockTest, test_range_locks_and_unlocks_heap_buffer)
{
    // Arrange
    void *buffer = malloc(4096U);
    ASSERT_NE(nullptr, buffer);

    // Act
    com_util_memory_lock_result_t lock_result = com_util_memory_lock_range(buffer, 4096U);
    com_util_memory_lock_result_t unlock_result = COM_UTIL_MEMORY_LOCK_SYSTEM_ERROR;
    if (lock_result == COM_UTIL_MEMORY_LOCK_OK)
    {
        unlock_result = com_util_memory_unlock_range(buffer, 4096U);
    }

    // Assert
    EXPECT_EQ(COM_UTIL_MEMORY_LOCK_OK, lock_result);
    EXPECT_EQ(COM_UTIL_MEMORY_LOCK_OK, unlock_result);

    free(buffer);
}

TEST_F(memoryLockTest, test_lock_self_rejects_invalid_arguments)
{
    // Arrange
    com_util_memory_lock_scope *scope = nullptr;
    com_util_memory_lock_self_options options = {};
    options.flags = COM_UTIL_MEMORY_LOCK_CURRENT;

    // Act & Assert
    EXPECT_EQ(COM_UTIL_MEMORY_LOCK_INVALID_ARGUMENT, com_util_memory_lock_self(NULL, &scope));
    EXPECT_EQ(COM_UTIL_MEMORY_LOCK_INVALID_ARGUMENT, com_util_memory_lock_self(&options, NULL));
    options.flags = 0;
    EXPECT_EQ(COM_UTIL_MEMORY_LOCK_INVALID_ARGUMENT, com_util_memory_lock_self(&options, &scope));
    options.flags = COM_UTIL_MEMORY_LOCK_CURRENT | 0x100;
    EXPECT_EQ(COM_UTIL_MEMORY_LOCK_INVALID_ARGUMENT, com_util_memory_lock_self(&options, &scope));
}

TEST_F(memoryLockTest, test_scope_release_accepts_null)
{
    EXPECT_EQ(COM_UTIL_MEMORY_LOCK_OK, com_util_memory_lock_scope_release(NULL));
}

TEST_F(memoryLockTest, test_lock_self_rejects_excessive_stack_prefault)
{
    // Arrange
    com_util_memory_lock_scope *scope = nullptr;
    com_util_memory_lock_self_options options = {};
    options.flags = COM_UTIL_MEMORY_LOCK_CURRENT;
    options.stack_prefault_bytes = static_cast<size_t>(-1);

    // Act & Assert
    EXPECT_EQ(COM_UTIL_MEMORY_LOCK_LIMIT_EXCEEDED, com_util_memory_lock_self(&options, &scope));
    EXPECT_EQ(nullptr, scope);
}

TEST_F(memoryLockTest, test_lock_self_allows_independent_scopes_from_multiple_threads)
{
    // Arrange
    self_lock_thread_result first = {};
    self_lock_thread_result second = {};

    // Act
    std::thread first_thread(lock_self_current_in_thread, &first);
    std::thread second_thread(lock_self_current_in_thread, &second);
    first_thread.join();
    second_thread.join();

    // Assert
    if ((first.result != COM_UTIL_MEMORY_LOCK_OK) || (second.result != COM_UTIL_MEMORY_LOCK_OK))
    {
        (void)com_util_memory_lock_scope_release(first.scope);
        (void)com_util_memory_lock_scope_release(second.scope);
        GTEST_SKIP() << "self memory lock is not available in this environment";
    }

    ASSERT_NE(nullptr, first.scope);
    ASSERT_NE(nullptr, second.scope);

    com_util_memory_lock_result_t first_release = COM_UTIL_MEMORY_LOCK_SYSTEM_ERROR;
    com_util_memory_lock_result_t second_release = COM_UTIL_MEMORY_LOCK_SYSTEM_ERROR;
    std::thread first_release_thread(release_self_scope_in_thread, first.scope, &first_release);
    std::thread second_release_thread(release_self_scope_in_thread, second.scope, &second_release);
    first_release_thread.join();
    second_release_thread.join();

    EXPECT_EQ(COM_UTIL_MEMORY_LOCK_OK, first_release);
    EXPECT_EQ(COM_UTIL_MEMORY_LOCK_OK, second_release);
}

#if defined(PLATFORM_WINDOWS)
TEST_F(memoryLockTest, test_lock_self_rejects_unsupported_flags_on_windows)
{
    // Arrange
    com_util_memory_lock_scope *scope = nullptr;
    com_util_memory_lock_self_options options = {};

    // Act & Assert
    options.flags = COM_UTIL_MEMORY_LOCK_FUTURE;
    EXPECT_EQ(COM_UTIL_MEMORY_LOCK_UNSUPPORTED, com_util_memory_lock_self(&options, &scope));
    options.flags = COM_UTIL_MEMORY_LOCK_ONFAULT;
    EXPECT_EQ(COM_UTIL_MEMORY_LOCK_UNSUPPORTED, com_util_memory_lock_self(&options, &scope));
    options.flags = COM_UTIL_MEMORY_LOCK_CURRENT | COM_UTIL_MEMORY_LOCK_FUTURE;
    EXPECT_EQ(COM_UTIL_MEMORY_LOCK_UNSUPPORTED, com_util_memory_lock_self(&options, &scope));
}
#endif
