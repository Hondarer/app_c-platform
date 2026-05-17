#include <testfw.h>
#include <mock_com_util.h>

#define DEFINE_SYNC_RET(rettype, name, call_args, ...) \
    rettype delegate_real_##name(__VA_ARGS__) \
    { \
        static auto real_fn = \
            reinterpret_cast<decltype(&name)>(resolveSharedSymbolOrExit(kLibComUtilName, #name)); \
        return real_fn call_args; \
    } \
    MOCK_WEAK_IMPL(rettype, name, __VA_ARGS__) \
    { \
        rettype rtc; \
        if (_mock_com_util != nullptr) \
        { \
            rtc = _mock_com_util->name call_args; \
        } \
        else \
        { \
            rtc = delegate_real_##name call_args; \
        } \
        if (getTraceLevel() > TRACE_NONE) \
        { \
            printf("  > %s -> %d\n", __func__, (int)rtc); \
        } \
        return rtc; \
    }

#define DEFINE_SYNC_VOID(name, call_args, ...) \
    void delegate_real_##name(__VA_ARGS__) \
    { \
        static auto real_fn = \
            reinterpret_cast<decltype(&name)>(resolveSharedSymbolOrExit(kLibComUtilName, #name)); \
        real_fn call_args; \
    } \
    MOCK_WEAK_IMPL(void, name, __VA_ARGS__) \
    { \
        if (_mock_com_util != nullptr) \
        { \
            _mock_com_util->name call_args; \
        } \
        else \
        { \
            delegate_real_##name call_args; \
        } \
        if (getTraceLevel() > TRACE_NONE) \
        { \
            printf("  > %s\n", __func__); \
        } \
    }

DEFINE_SYNC_RET(com_util_sync_result_t, com_util_local_lock_create, (mtx), com_util_local_lock_t **mtx)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_local_lock_lock, (mtx, timeout_ms), com_util_local_lock_t *mtx, int timeout_ms)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_local_lock_try_lock, (mtx), com_util_local_lock_t *mtx)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_local_lock_unlock, (mtx), com_util_local_lock_t *mtx)
DEFINE_SYNC_VOID(com_util_local_lock_destroy, (mtx), com_util_local_lock_t *mtx)

DEFINE_SYNC_RET(com_util_sync_result_t, com_util_condvar_create, (cv), com_util_condvar_t **cv)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_condvar_wait, (cv, mtx, timeout_ms), com_util_condvar_t *cv, com_util_local_lock_t *mtx, int timeout_ms)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_condvar_signal, (cv), com_util_condvar_t *cv)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_condvar_broadcast, (cv), com_util_condvar_t *cv)
DEFINE_SYNC_VOID(com_util_condvar_destroy, (cv), com_util_condvar_t *cv)

DEFINE_SYNC_RET(com_util_sync_result_t, com_util_local_rwlock_create, (rwlock), com_util_local_rwlock_t **rwlock)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_local_rwlock_lock_shared, (rwlock, timeout_ms), com_util_local_rwlock_t *rwlock, int timeout_ms)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_local_rwlock_try_lock_shared, (rwlock), com_util_local_rwlock_t *rwlock)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_local_rwlock_lock_exclusive, (rwlock, timeout_ms), com_util_local_rwlock_t *rwlock, int timeout_ms)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_local_rwlock_try_lock_exclusive, (rwlock), com_util_local_rwlock_t *rwlock)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_local_rwlock_unlock_shared, (rwlock), com_util_local_rwlock_t *rwlock)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_local_rwlock_unlock_exclusive, (rwlock), com_util_local_rwlock_t *rwlock)
DEFINE_SYNC_VOID(com_util_local_rwlock_destroy, (rwlock), com_util_local_rwlock_t *rwlock)

DEFINE_SYNC_RET(com_util_sync_result_t, com_util_thread_create, (thread, func, arg), com_util_thread_t **thread, com_util_thread_func_t func, void *arg)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_thread_join, (thread, timeout_ms), com_util_thread_t *thread, int timeout_ms)
DEFINE_SYNC_VOID(com_util_thread_detach, (thread), com_util_thread_t *thread)

DEFINE_SYNC_RET(com_util_sync_result_t, com_util_interprocess_lock_open, (identity, lock), const char *identity, com_util_interprocess_lock_t **lock)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_interprocess_lock_import_descriptor, (descriptor, descriptor_size, lock), const void *descriptor, size_t descriptor_size, com_util_interprocess_lock_t **lock)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_interprocess_lock_export_descriptor, (lock, descriptor, descriptor_size), const com_util_interprocess_lock_t *lock, void *descriptor, size_t *descriptor_size)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_interprocess_lock_lock, (lock, timeout_ms), com_util_interprocess_lock_t *lock, int timeout_ms)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_interprocess_lock_try_lock, (lock), com_util_interprocess_lock_t *lock)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_interprocess_lock_unlock, (lock), com_util_interprocess_lock_t *lock)
DEFINE_SYNC_VOID(com_util_interprocess_lock_destroy, (lock), com_util_interprocess_lock_t *lock)

DEFINE_SYNC_RET(com_util_sync_result_t, com_util_interprocess_rwlock_open, (identity, lock), const char *identity, com_util_interprocess_rwlock_t **lock)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_interprocess_rwlock_import_descriptor, (descriptor, descriptor_size, lock), const void *descriptor, size_t descriptor_size, com_util_interprocess_rwlock_t **lock)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_interprocess_rwlock_export_descriptor, (lock, descriptor, descriptor_size), const com_util_interprocess_rwlock_t *lock, void *descriptor, size_t *descriptor_size)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_interprocess_rwlock_lock_shared, (lock, timeout_ms), com_util_interprocess_rwlock_t *lock, int timeout_ms)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_interprocess_rwlock_try_lock_shared, (lock), com_util_interprocess_rwlock_t *lock)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_interprocess_rwlock_lock_exclusive, (lock, timeout_ms), com_util_interprocess_rwlock_t *lock, int timeout_ms)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_interprocess_rwlock_try_lock_exclusive, (lock), com_util_interprocess_rwlock_t *lock)
DEFINE_SYNC_RET(com_util_sync_result_t, com_util_interprocess_rwlock_unlock, (lock), com_util_interprocess_rwlock_t *lock)
DEFINE_SYNC_VOID(com_util_interprocess_rwlock_destroy, (lock), com_util_interprocess_rwlock_t *lock)
