// memory_lock.c の static 関数と内部状態へテスト用アクセサーを追加する
#ifndef _IN_TEST_SRC
    #include "memory_lock.c"
#endif /* _IN_TEST_SRC */

#include "memory_lock.inject.h"

#if defined(PLATFORM_LINUX)
int test_memory_lock_internal_lock(void)
{
    return memory_lock_lock();
}

void test_memory_lock_internal_unlock(void)
{
    memory_lock_unlock();
}

int test_memory_lock_convert_flags(const int flags, int *native_flags)
{
    return convert_flags_to_mlockall_flags(flags, native_flags);
}

int test_memory_lock_prefault_stack(const size_t bytes)
{
    return prefault_stack(bytes);
}

void test_memory_lock_prefault_stack_recursive(const size_t bytes)
{
    prefault_stack_recursive(bytes);
}

com_util_local_lock *test_memory_lock_get_internal_lock(void)
{
    return s_memory_lock_lock;
}

int test_memory_lock_get_once_state(void)
{
    return s_memory_lock_lock_once.state;
}

void test_memory_lock_set_internal_lock(com_util_local_lock *lock, const int once_state)
{
    s_memory_lock_lock = lock;
    s_memory_lock_lock_once.state = once_state;
}

com_util_memory_lock_scope *test_memory_lock_create_scope(const int locked_all)
{
    com_util_memory_lock_scope *scope = (com_util_memory_lock_scope *)calloc(1U, sizeof(*scope));

    if (scope != NULL)
    {
        scope->locked_all = locked_all;
    }
    return scope;
}

void test_memory_lock_set_scope_count(const size_t count)
{
    s_linux_self_lock_scope_count = count;
}
#endif /* PLATFORM_LINUX */
