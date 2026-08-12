// memory_lock.c の static 関数と内部状態へテストからアクセスするための宣言
#ifndef MEMORY_LOCK_INJECT_H
#define MEMORY_LOCK_INJECT_H

#include <com_util/runtime/memory_lock.h>
#include <com_util/sync/sync.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    extern int test_memory_lock_internal_lock(void);
    extern void test_memory_lock_internal_unlock(void);
    extern int test_memory_lock_convert_flags(int flags, int *native_flags);
    extern int test_memory_lock_prefault_stack(size_t bytes);
    extern void test_memory_lock_prefault_stack_recursive(size_t bytes);
    extern com_util_local_lock *test_memory_lock_get_internal_lock(void);
    extern int test_memory_lock_get_once_state(void);
    extern void test_memory_lock_set_internal_lock(com_util_local_lock *lock, int once_state);
    extern com_util_memory_lock_scope *test_memory_lock_create_scope(int locked_all);
    extern void test_memory_lock_set_scope_count(size_t count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MEMORY_LOCK_INJECT_H */
