/**
 *******************************************************************************
 *  @file           sync_windows.c
 *  @brief          Windows 向けスレッド・同期プリミティブ実装。
 *******************************************************************************
 */

#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)

    #include <process.h>
    #include <stdlib.h>
    #include <string.h>

    #include <com_util/base/windows_sdk.h>
    #include <com_util/sync/sync.h>

    #define INTERPROCESS_SYNC_DESCRIPTOR_HEADER_SIZE 20U
    #define INTERPROCESS_SYNC_RANGE_LOW              1U
    #define INTERPROCESS_SYNC_RANGE_HIGH             0U
    #define INTERPROCESS_SYNC_KIND_LOCK              1U
    #define INTERPROCESS_SYNC_KIND_RWLOCK            2U

struct com_util_local_lock
{
    SRWLOCK native;
};

struct com_util_condvar
{
    CONDITION_VARIABLE native;
};

struct com_util_local_rwlock
{
    CRITICAL_SECTION mutex;
    CONDITION_VARIABLE readers_cv;
    CONDITION_VARIABLE writers_cv;
    unsigned int active_readers;
    unsigned int waiting_writers;
    int writer_active;
};

struct com_util_thread
{
    HANDLE native;
};

struct com_util_interprocess_lock
{
    HANDLE handle;
    char *identity;
    int locked;
};

struct com_util_interprocess_rwlock
{
    HANDLE handle;
    char *identity;
    int locked;
};

struct com_util_thread_start_ctx
{
    com_util_thread_func_t func;
    void *arg;
};

static unsigned __stdcall thread_start_proc(void *opaque)
{
    struct com_util_thread_start_ctx *ctx = (struct com_util_thread_start_ctx *)opaque;
    com_util_thread_func_t func = ctx->func;
    void *arg = ctx->arg;

    free(ctx);
    func(arg);
    return 0U;
}

static com_util_sync_result_t wait_status_to_result(DWORD status)
{
    if (status == WAIT_OBJECT_0)
    {
        return COM_UTIL_SYNC_OK;
    }
    if (status == WAIT_TIMEOUT)
    {
        return COM_UTIL_SYNC_TIMEOUT;
    }
    return COM_UTIL_SYNC_SYSTEM_ERROR;
}

static char *dup_string(const char *s)
{
    size_t len;
    char *copy;

    len = strlen(s);
    copy = (char *)malloc(len + 1U);
    if (copy != NULL)
    {
        memcpy(copy, s, len + 1U);
    }
    return copy;
}

static com_util_sync_result_t app_lock_open_identity(const char *identity, com_util_interprocess_rwlock **lock)
{
    com_util_interprocess_rwlock *new_lock;
    HANDLE handle;
    char *identity_copy;

    if (identity == NULL || identity[0] == '\0' || lock == NULL)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    handle = CreateFileA(identity, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE)
    {
        return COM_UTIL_SYNC_SYSTEM_ERROR;
    }
    identity_copy = dup_string(identity);
    if (identity_copy == NULL)
    {
        CloseHandle(handle);
        return COM_UTIL_SYNC_SYSTEM_ERROR;
    }
    new_lock = (com_util_interprocess_rwlock *)calloc(1, sizeof(*new_lock));
    if (new_lock == NULL)
    {
        free(identity_copy);
        CloseHandle(handle);
        return COM_UTIL_SYNC_SYSTEM_ERROR;
    }
    new_lock->handle = handle;
    new_lock->identity = identity_copy;
    *lock = new_lock;
    return COM_UTIL_SYNC_OK;
}

static com_util_sync_result_t interprocess_lock_open_identity(const char *identity, com_util_interprocess_lock **lock)
{
    com_util_interprocess_lock *new_lock;
    HANDLE handle;
    char *identity_copy;

    if (identity == NULL || identity[0] == '\0' || lock == NULL)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    handle = CreateFileA(identity, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE)
    {
        return COM_UTIL_SYNC_SYSTEM_ERROR;
    }
    identity_copy = dup_string(identity);
    if (identity_copy == NULL)
    {
        CloseHandle(handle);
        return COM_UTIL_SYNC_SYSTEM_ERROR;
    }
    new_lock = (com_util_interprocess_lock *)calloc(1, sizeof(*new_lock));
    if (new_lock == NULL)
    {
        free(identity_copy);
        CloseHandle(handle);
        return COM_UTIL_SYNC_SYSTEM_ERROR;
    }
    new_lock->handle = handle;
    new_lock->identity = identity_copy;
    *lock = new_lock;
    return COM_UTIL_SYNC_OK;
}

static com_util_sync_result_t app_lock_take(com_util_interprocess_rwlock *lock, DWORD flags, int timeout_ms)
{
    ULONGLONG deadline;
    OVERLAPPED ov;

    if (lock == NULL || lock->locked)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    deadline = GetTickCount64() + (ULONGLONG)timeout_ms;
    memset(&ov, 0, sizeof(ov));
    do
    {
        if (LockFileEx(lock->handle, flags | LOCKFILE_FAIL_IMMEDIATELY, 0, INTERPROCESS_SYNC_RANGE_LOW,
                       INTERPROCESS_SYNC_RANGE_HIGH, &ov))
        {
            lock->locked = 1;
            return COM_UTIL_SYNC_OK;
        }
        if (timeout_ms == COM_UTIL_SYNC_NO_WAIT)
        {
            return COM_UTIL_SYNC_BUSY;
        }
        if (GetLastError() != ERROR_LOCK_VIOLATION)
        {
            return COM_UTIL_SYNC_SYSTEM_ERROR;
        }
        Sleep(1);
    } while (timeout_ms == COM_UTIL_SYNC_WAIT_FOREVER || GetTickCount64() < deadline);

    return COM_UTIL_SYNC_TIMEOUT;
}

static com_util_sync_result_t interprocess_lock_take(com_util_interprocess_lock *lock, int timeout_ms)
{
    ULONGLONG deadline;
    OVERLAPPED ov;

    if (lock == NULL || lock->locked)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    deadline = GetTickCount64() + (ULONGLONG)timeout_ms;
    memset(&ov, 0, sizeof(ov));
    do
    {
        if (LockFileEx(lock->handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0,
                       INTERPROCESS_SYNC_RANGE_LOW, INTERPROCESS_SYNC_RANGE_HIGH, &ov))
        {
            lock->locked = 1;
            return COM_UTIL_SYNC_OK;
        }
        if (timeout_ms == COM_UTIL_SYNC_NO_WAIT)
        {
            return COM_UTIL_SYNC_BUSY;
        }
        if (GetLastError() != ERROR_LOCK_VIOLATION)
        {
            return COM_UTIL_SYNC_SYSTEM_ERROR;
        }
        Sleep(1);
    } while (timeout_ms == COM_UTIL_SYNC_WAIT_FOREVER || GetTickCount64() < deadline);

    return COM_UTIL_SYNC_TIMEOUT;
}

com_util_sync_result_t com_util_local_lock_create(com_util_local_lock **mtx)
{
    com_util_local_lock *new_mtx;

    if (mtx == NULL)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    new_mtx = (com_util_local_lock *)calloc(1, sizeof(*new_mtx));
    if (new_mtx == NULL)
    {
        return COM_UTIL_SYNC_SYSTEM_ERROR;
    }
    InitializeSRWLock(&new_mtx->native);
    *mtx = new_mtx;
    return COM_UTIL_SYNC_OK;
}

com_util_sync_result_t com_util_local_lock_lock(com_util_local_lock *mtx, int timeout_ms)
{
    ULONGLONG deadline;

    if (mtx == NULL || timeout_ms < 0)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    if (timeout_ms == COM_UTIL_SYNC_WAIT_FOREVER)
    {
        AcquireSRWLockExclusive(&mtx->native);
        return COM_UTIL_SYNC_OK;
    }
    deadline = GetTickCount64() + (ULONGLONG)timeout_ms;
    do
    {
        if (TryAcquireSRWLockExclusive(&mtx->native))
        {
            return COM_UTIL_SYNC_OK;
        }
        if (timeout_ms == COM_UTIL_SYNC_NO_WAIT)
        {
            return COM_UTIL_SYNC_BUSY;
        }
        Sleep(1);
    } while (GetTickCount64() < deadline);
    return COM_UTIL_SYNC_TIMEOUT;
}

com_util_sync_result_t com_util_local_lock_try_lock(com_util_local_lock *mtx)
{
    return com_util_local_lock_lock(mtx, COM_UTIL_SYNC_NO_WAIT);
}

com_util_sync_result_t com_util_local_lock_unlock(com_util_local_lock *mtx)
{
    if (mtx == NULL)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    ReleaseSRWLockExclusive(&mtx->native);
    return COM_UTIL_SYNC_OK;
}

void com_util_local_lock_destroy(com_util_local_lock *mtx)
{
    free(mtx);
}

com_util_sync_result_t com_util_condvar_create(com_util_condvar **cv)
{
    com_util_condvar *new_cv;

    if (cv == NULL)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    new_cv = (com_util_condvar *)calloc(1, sizeof(*new_cv));
    if (new_cv == NULL)
    {
        return COM_UTIL_SYNC_SYSTEM_ERROR;
    }
    InitializeConditionVariable(&new_cv->native);
    *cv = new_cv;
    return COM_UTIL_SYNC_OK;
}

com_util_sync_result_t com_util_condvar_wait(com_util_condvar *cv, com_util_local_lock *mtx, int timeout_ms)
{
    DWORD wait_ms;

    if (cv == NULL || mtx == NULL || timeout_ms < 0)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    if (timeout_ms == COM_UTIL_SYNC_WAIT_FOREVER)
    {
        wait_ms = INFINITE;
    }
    else
    {
        wait_ms = (DWORD)timeout_ms;
    }
    if (SleepConditionVariableSRW(&cv->native, &mtx->native, wait_ms, 0))
    {
        return COM_UTIL_SYNC_OK;
    }
    if (GetLastError() == ERROR_TIMEOUT)
    {
        return COM_UTIL_SYNC_TIMEOUT;
    }
    else
    {
        return COM_UTIL_SYNC_SYSTEM_ERROR;
    }
}

com_util_sync_result_t com_util_condvar_signal(com_util_condvar *cv)
{
    if (cv == NULL)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    WakeConditionVariable(&cv->native);
    return COM_UTIL_SYNC_OK;
}

com_util_sync_result_t com_util_condvar_broadcast(com_util_condvar *cv)
{
    if (cv == NULL)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    WakeAllConditionVariable(&cv->native);
    return COM_UTIL_SYNC_OK;
}

void com_util_condvar_destroy(com_util_condvar *cv)
{
    free(cv);
}

com_util_sync_result_t com_util_local_rwlock_create(com_util_local_rwlock **rwlock)
{
    com_util_local_rwlock *new_lock;

    if (rwlock == NULL)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    new_lock = (com_util_local_rwlock *)calloc(1, sizeof(*new_lock));
    if (new_lock == NULL)
    {
        return COM_UTIL_SYNC_SYSTEM_ERROR;
    }
    InitializeCriticalSection(&new_lock->mutex);
    InitializeConditionVariable(&new_lock->readers_cv);
    InitializeConditionVariable(&new_lock->writers_cv);
    *rwlock = new_lock;
    return COM_UTIL_SYNC_OK;
}

com_util_sync_result_t com_util_local_rwlock_lock_shared(com_util_local_rwlock *rwlock, int timeout_ms)
{
    DWORD wait_ms;

    if (rwlock == NULL || timeout_ms < 0)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    if (timeout_ms == COM_UTIL_SYNC_WAIT_FOREVER)
    {
        wait_ms = INFINITE;
    }
    else
    {
        wait_ms = (DWORD)timeout_ms;
    }
    EnterCriticalSection(&rwlock->mutex);
    while (rwlock->writer_active || rwlock->waiting_writers > 0)
    {
        if (timeout_ms == COM_UTIL_SYNC_NO_WAIT)
        {
            LeaveCriticalSection(&rwlock->mutex);
            return COM_UTIL_SYNC_BUSY;
        }
        if (!SleepConditionVariableCS(&rwlock->readers_cv, &rwlock->mutex, wait_ms))
        {
            LeaveCriticalSection(&rwlock->mutex);
            if (GetLastError() == ERROR_TIMEOUT)
            {
                return COM_UTIL_SYNC_TIMEOUT;
            }
            else
            {
                return COM_UTIL_SYNC_SYSTEM_ERROR;
            }
        }
    }
    rwlock->active_readers++;
    LeaveCriticalSection(&rwlock->mutex);
    return COM_UTIL_SYNC_OK;
}

com_util_sync_result_t com_util_local_rwlock_try_lock_shared(com_util_local_rwlock *rwlock)
{
    return com_util_local_rwlock_lock_shared(rwlock, COM_UTIL_SYNC_NO_WAIT);
}

com_util_sync_result_t com_util_local_rwlock_lock_exclusive(com_util_local_rwlock *rwlock, int timeout_ms)
{
    DWORD wait_ms;

    if (rwlock == NULL || timeout_ms < 0)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    if (timeout_ms == COM_UTIL_SYNC_WAIT_FOREVER)
    {
        wait_ms = INFINITE;
    }
    else
    {
        wait_ms = (DWORD)timeout_ms;
    }
    EnterCriticalSection(&rwlock->mutex);
    rwlock->waiting_writers++;
    while (rwlock->writer_active || rwlock->active_readers > 0)
    {
        if (timeout_ms == COM_UTIL_SYNC_NO_WAIT)
        {
            rwlock->waiting_writers--;
            LeaveCriticalSection(&rwlock->mutex);
            return COM_UTIL_SYNC_BUSY;
        }
        if (!SleepConditionVariableCS(&rwlock->writers_cv, &rwlock->mutex, wait_ms))
        {
            rwlock->waiting_writers--;
            LeaveCriticalSection(&rwlock->mutex);
            if (GetLastError() == ERROR_TIMEOUT)
            {
                return COM_UTIL_SYNC_TIMEOUT;
            }
            else
            {
                return COM_UTIL_SYNC_SYSTEM_ERROR;
            }
        }
    }
    rwlock->waiting_writers--;
    rwlock->writer_active = 1;
    LeaveCriticalSection(&rwlock->mutex);
    return COM_UTIL_SYNC_OK;
}

com_util_sync_result_t com_util_local_rwlock_try_lock_exclusive(com_util_local_rwlock *rwlock)
{
    return com_util_local_rwlock_lock_exclusive(rwlock, COM_UTIL_SYNC_NO_WAIT);
}

com_util_sync_result_t com_util_local_rwlock_unlock_shared(com_util_local_rwlock *rwlock)
{
    if (rwlock == NULL)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    EnterCriticalSection(&rwlock->mutex);
    if (rwlock->active_readers == 0)
    {
        LeaveCriticalSection(&rwlock->mutex);
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    rwlock->active_readers--;
    if (rwlock->active_readers == 0 && rwlock->waiting_writers > 0)
    {
        WakeConditionVariable(&rwlock->writers_cv);
    }
    LeaveCriticalSection(&rwlock->mutex);
    return COM_UTIL_SYNC_OK;
}

com_util_sync_result_t com_util_local_rwlock_unlock_exclusive(com_util_local_rwlock *rwlock)
{
    if (rwlock == NULL)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    EnterCriticalSection(&rwlock->mutex);
    if (!rwlock->writer_active)
    {
        LeaveCriticalSection(&rwlock->mutex);
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    rwlock->writer_active = 0;
    if (rwlock->waiting_writers > 0)
    {
        WakeConditionVariable(&rwlock->writers_cv);
    }
    else
    {
        WakeAllConditionVariable(&rwlock->readers_cv);
    }
    LeaveCriticalSection(&rwlock->mutex);
    return COM_UTIL_SYNC_OK;
}

void com_util_local_rwlock_destroy(com_util_local_rwlock *rwlock)
{
    if (rwlock != NULL)
    {
        DeleteCriticalSection(&rwlock->mutex);
        free(rwlock);
    }
}

com_util_sync_result_t com_util_thread_create(com_util_thread **thread, com_util_thread_func_t func, void *arg)
{
    struct com_util_thread_start_ctx *ctx;
    com_util_thread *new_thread;
    uintptr_t handle;

    if (thread == NULL || func == NULL)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    new_thread = (com_util_thread *)calloc(1, sizeof(*new_thread));
    ctx = (struct com_util_thread_start_ctx *)malloc(sizeof(*ctx));
    if (new_thread == NULL || ctx == NULL)
    {
        free(new_thread);
        free(ctx);
        return COM_UTIL_SYNC_SYSTEM_ERROR;
    }
    ctx->func = func;
    ctx->arg = arg;
    handle = _beginthreadex(NULL, 0U, thread_start_proc, ctx, 0U, NULL);
    if (handle == 0U)
    {
        free(ctx);
        free(new_thread);
        return COM_UTIL_SYNC_SYSTEM_ERROR;
    }
    new_thread->native = (HANDLE)handle;
    *thread = new_thread;
    return COM_UTIL_SYNC_OK;
}

com_util_sync_result_t com_util_thread_join(com_util_thread *thread, int timeout_ms)
{
    DWORD status;
    DWORD wait_ms;

    if (thread == NULL || timeout_ms < 0)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    if (timeout_ms == COM_UTIL_SYNC_WAIT_FOREVER)
    {
        wait_ms = INFINITE;
    }
    else
    {
        wait_ms = (DWORD)timeout_ms;
    }
    status = WaitForSingleObject(thread->native, wait_ms);
    if (status == WAIT_OBJECT_0)
    {
        CloseHandle(thread->native);
        free(thread);
        return COM_UTIL_SYNC_OK;
    }
    return wait_status_to_result(status);
}

void com_util_thread_detach(com_util_thread *thread)
{
    if (thread != NULL)
    {
        CloseHandle(thread->native);
        free(thread);
    }
}

com_util_sync_result_t com_util_interprocess_lock_open(const char *identity, com_util_interprocess_lock **lock)
{
    return interprocess_lock_open_identity(identity, lock);
}

com_util_sync_result_t com_util_interprocess_lock_export_descriptor(const com_util_interprocess_lock *lock,
                                                                    void *descriptor, size_t *descriptor_size)
{
    uint8_t *out;
    size_t identity_len;
    size_t required;

    if (lock == NULL || descriptor_size == NULL)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    identity_len = strlen(lock->identity);
    required = INTERPROCESS_SYNC_DESCRIPTOR_HEADER_SIZE + identity_len;
    if (descriptor == NULL || *descriptor_size < required)
    {
        *descriptor_size = required;
        return COM_UTIL_SYNC_BUFFER_TOO_SMALL;
    }
    out = (uint8_t *)descriptor;
    memcpy(out, "CULK", 4);
    out[4] = 1U;
    out[5] = INTERPROCESS_SYNC_KIND_LOCK;
    out[6] = (uint8_t)COM_UTIL_INTERPROCESS_SYNC_BACKEND_LOCK_FILE;
    out[7] = 0U;
    out[8] = (uint8_t)(identity_len & 0xffU);
    out[9] = (uint8_t)((identity_len >> 8) & 0xffU);
    out[10] = (uint8_t)((identity_len >> 16) & 0xffU);
    out[11] = (uint8_t)((identity_len >> 24) & 0xffU);
    memset(out + 12, 0, 8);
    memcpy(out + INTERPROCESS_SYNC_DESCRIPTOR_HEADER_SIZE, lock->identity, identity_len);
    *descriptor_size = required;
    return COM_UTIL_SYNC_OK;
}

com_util_sync_result_t com_util_interprocess_lock_import_descriptor(const void *descriptor, size_t descriptor_size,
                                                                    com_util_interprocess_lock **lock)
{
    const uint8_t *in = (const uint8_t *)descriptor;
    uint32_t identity_len;
    char *identity;
    com_util_sync_result_t result;

    if (descriptor == NULL || lock == NULL)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    if (descriptor_size < INTERPROCESS_SYNC_DESCRIPTOR_HEADER_SIZE || memcmp(in, "CULK", 4) != 0 || in[4] != 1U ||
        in[5] != INTERPROCESS_SYNC_KIND_LOCK || in[6] != (uint8_t)COM_UTIL_INTERPROCESS_SYNC_BACKEND_LOCK_FILE)
    {
        return COM_UTIL_SYNC_CORRUPT_DESCRIPTOR;
    }
    identity_len = (uint32_t)in[8] | ((uint32_t)in[9] << 8) | ((uint32_t)in[10] << 16) | ((uint32_t)in[11] << 24);
    if (identity_len == 0 || descriptor_size != INTERPROCESS_SYNC_DESCRIPTOR_HEADER_SIZE + (size_t)identity_len)
    {
        return COM_UTIL_SYNC_CORRUPT_DESCRIPTOR;
    }
    identity = (char *)malloc((size_t)identity_len + 1U);
    if (identity == NULL)
    {
        return COM_UTIL_SYNC_SYSTEM_ERROR;
    }
    memcpy(identity, in + INTERPROCESS_SYNC_DESCRIPTOR_HEADER_SIZE, identity_len);
    identity[identity_len] = '\0';
    result = interprocess_lock_open_identity(identity, lock);
    free(identity);
    return result;
}

com_util_sync_result_t com_util_interprocess_lock_lock(com_util_interprocess_lock *lock, int timeout_ms)
{
    if (timeout_ms < 0)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    return interprocess_lock_take(lock, timeout_ms);
}

com_util_sync_result_t com_util_interprocess_lock_try_lock(com_util_interprocess_lock *lock)
{
    return com_util_interprocess_lock_lock(lock, COM_UTIL_SYNC_NO_WAIT);
}

com_util_sync_result_t com_util_interprocess_lock_unlock(com_util_interprocess_lock *lock)
{
    OVERLAPPED ov;

    if (lock == NULL || !lock->locked)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    memset(&ov, 0, sizeof(ov));
    if (!UnlockFileEx(lock->handle, 0, INTERPROCESS_SYNC_RANGE_LOW, INTERPROCESS_SYNC_RANGE_HIGH, &ov))
    {
        return COM_UTIL_SYNC_SYSTEM_ERROR;
    }
    lock->locked = 0;
    return COM_UTIL_SYNC_OK;
}

void com_util_interprocess_lock_destroy(com_util_interprocess_lock *lock)
{
    if (lock != NULL)
    {
        if (lock->locked)
        {
            (void)com_util_interprocess_lock_unlock(lock);
        }
        CloseHandle(lock->handle);
        free(lock->identity);
        free(lock);
    }
}

com_util_sync_result_t com_util_interprocess_rwlock_open(const char *identity, com_util_interprocess_rwlock **lock)
{
    return app_lock_open_identity(identity, lock);
}

com_util_sync_result_t com_util_interprocess_rwlock_export_descriptor(const com_util_interprocess_rwlock *lock,
                                                                      void *descriptor, size_t *descriptor_size)
{
    uint8_t *out;
    size_t identity_len;
    size_t required;

    if (lock == NULL || descriptor_size == NULL)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    identity_len = strlen(lock->identity);
    required = INTERPROCESS_SYNC_DESCRIPTOR_HEADER_SIZE + identity_len;
    if (descriptor == NULL || *descriptor_size < required)
    {
        *descriptor_size = required;
        return COM_UTIL_SYNC_BUFFER_TOO_SMALL;
    }
    out = (uint8_t *)descriptor;
    memcpy(out, "CULK", 4);
    out[4] = 1U;
    out[5] = INTERPROCESS_SYNC_KIND_RWLOCK;
    out[6] = (uint8_t)COM_UTIL_INTERPROCESS_SYNC_BACKEND_LOCK_FILE;
    out[7] = 0U;
    out[8] = (uint8_t)(identity_len & 0xffU);
    out[9] = (uint8_t)((identity_len >> 8) & 0xffU);
    out[10] = (uint8_t)((identity_len >> 16) & 0xffU);
    out[11] = (uint8_t)((identity_len >> 24) & 0xffU);
    memset(out + 12, 0, 8);
    memcpy(out + INTERPROCESS_SYNC_DESCRIPTOR_HEADER_SIZE, lock->identity, identity_len);
    *descriptor_size = required;
    return COM_UTIL_SYNC_OK;
}

com_util_sync_result_t com_util_interprocess_rwlock_import_descriptor(const void *descriptor, size_t descriptor_size,
                                                                      com_util_interprocess_rwlock **lock)
{
    const uint8_t *in = (const uint8_t *)descriptor;
    uint32_t identity_len;
    char *identity;
    com_util_sync_result_t result;

    if (descriptor == NULL || lock == NULL)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    if (descriptor_size < INTERPROCESS_SYNC_DESCRIPTOR_HEADER_SIZE || memcmp(in, "CULK", 4) != 0 || in[4] != 1U ||
        in[5] != INTERPROCESS_SYNC_KIND_RWLOCK || in[6] != (uint8_t)COM_UTIL_INTERPROCESS_SYNC_BACKEND_LOCK_FILE)
    {
        return COM_UTIL_SYNC_CORRUPT_DESCRIPTOR;
    }
    identity_len = (uint32_t)in[8] | ((uint32_t)in[9] << 8) | ((uint32_t)in[10] << 16) | ((uint32_t)in[11] << 24);
    if (identity_len == 0 || descriptor_size != INTERPROCESS_SYNC_DESCRIPTOR_HEADER_SIZE + (size_t)identity_len)
    {
        return COM_UTIL_SYNC_CORRUPT_DESCRIPTOR;
    }
    identity = (char *)malloc((size_t)identity_len + 1U);
    if (identity == NULL)
    {
        return COM_UTIL_SYNC_SYSTEM_ERROR;
    }
    memcpy(identity, in + INTERPROCESS_SYNC_DESCRIPTOR_HEADER_SIZE, identity_len);
    identity[identity_len] = '\0';
    result = app_lock_open_identity(identity, lock);
    free(identity);
    return result;
}

com_util_sync_result_t com_util_interprocess_rwlock_lock_shared(com_util_interprocess_rwlock *lock, int timeout_ms)
{
    if (timeout_ms < 0)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    return app_lock_take(lock, 0U, timeout_ms);
}

com_util_sync_result_t com_util_interprocess_rwlock_try_lock_shared(com_util_interprocess_rwlock *lock)
{
    return com_util_interprocess_rwlock_lock_shared(lock, COM_UTIL_SYNC_NO_WAIT);
}

com_util_sync_result_t com_util_interprocess_rwlock_lock_exclusive(com_util_interprocess_rwlock *lock, int timeout_ms)
{
    if (timeout_ms < 0)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    return app_lock_take(lock, LOCKFILE_EXCLUSIVE_LOCK, timeout_ms);
}

com_util_sync_result_t com_util_interprocess_rwlock_try_lock_exclusive(com_util_interprocess_rwlock *lock)
{
    return com_util_interprocess_rwlock_lock_exclusive(lock, COM_UTIL_SYNC_NO_WAIT);
}

com_util_sync_result_t com_util_interprocess_rwlock_unlock(com_util_interprocess_rwlock *lock)
{
    OVERLAPPED ov;

    if (lock == NULL || !lock->locked)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    memset(&ov, 0, sizeof(ov));
    if (!UnlockFileEx(lock->handle, 0, INTERPROCESS_SYNC_RANGE_LOW, INTERPROCESS_SYNC_RANGE_HIGH, &ov))
    {
        return COM_UTIL_SYNC_SYSTEM_ERROR;
    }
    lock->locked = 0;
    return COM_UTIL_SYNC_OK;
}

void com_util_interprocess_rwlock_destroy(com_util_interprocess_rwlock *lock)
{
    if (lock != NULL)
    {
        if (lock->locked)
        {
            (void)com_util_interprocess_rwlock_unlock(lock);
        }
        CloseHandle(lock->handle);
        free(lock->identity);
        free(lock);
    }
}

void com_util_call_once(com_util_once_flag *flag, void (*func)(void))
{
    if (flag == NULL || func == NULL)
    {
        return;
    }
    if (InterlockedCompareExchange((volatile LONG *)&flag->state, 1, 0) == 0)
    {
        func();
        InterlockedExchange((volatile LONG *)&flag->state, 2);
        return;
    }
    while (InterlockedCompareExchange((volatile LONG *)&flag->state, 2, 2) != 2)
    {
        SwitchToThread();
    }
}

void com_util_sleep_ms(int ms)
{
    if (ms <= 0)
    {
        return;
    }
    Sleep((DWORD)ms);
}

#endif /* PLATFORM_WINDOWS */
