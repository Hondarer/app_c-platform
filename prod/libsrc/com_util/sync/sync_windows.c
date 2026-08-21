/**
 *******************************************************************************
 *  @file           sync_windows.c
 *  @brief          Windows 向けのスレッドと同期プリミティブを実装します。
 *******************************************************************************
 */

#include <com_util/base/platform.h>
#include <com_util/crt/stdlib.h>

#if defined(PLATFORM_WINDOWS)

    #include <process.h>
    #include <stdlib.h>
    #include <string.h>

    #include <com_util/base/windows_sdk.h>
    #include <com_util/sync/sync.h>
    #include <com_util/sync/sync_descriptor.h>
    #include <com_util/win32/win32.h>

    #define INTERPROCESS_SYNC_RANGE_LOW  1U
    #define INTERPROCESS_SYNC_RANGE_HIGH 0U

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
    com_util_thread_fn func;
    void *arg;
};

static unsigned __stdcall thread_start_proc(void *opaque)
{
    struct com_util_thread_start_ctx *ctx = (struct com_util_thread_start_ctx *)opaque;
    com_util_thread_fn func = ctx->func;
    void *arg = ctx->arg;

    com_util_free(ctx);
    func(arg);
    return 0U;
}

static int wait_status_to_result(DWORD status)
{
    if (status == WAIT_OBJECT_0)
    {
        return COM_UTIL_OK;
    }
    if (status == WAIT_TIMEOUT)
    {
        return COM_UTIL_ERR_TIMEOUT;
    }
    return COM_UTIL_ERR_UNKNOWN;
}

static char *dup_string(const char *s)
{
    size_t len;
    char *copy;

    len = strlen(s);
    copy = (char *)com_util_malloc(len + 1U);
    if (copy != NULL)
    {
        memcpy(copy, s, len + 1U);
    }
    return copy;
}

static int app_lock_open_identity(const char *identity, com_util_interprocess_rwlock **lock)
{
    com_util_interprocess_rwlock *new_lock;
    HANDLE handle;
    char *identity_copy;

    if (identity == NULL || identity[0] == '\0' || lock == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    handle = CreateFileU(identity, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE)
    {
        return COM_UTIL_ERR_UNKNOWN;
    }
    identity_copy = dup_string(identity);
    if (identity_copy == NULL)
    {
        CloseHandle(handle);
        return COM_UTIL_ERR_UNKNOWN;
    }
    new_lock = (com_util_interprocess_rwlock *)com_util_calloc(1, sizeof(*new_lock));
    if (new_lock == NULL)
    {
        com_util_free(identity_copy);
        CloseHandle(handle);
        return COM_UTIL_ERR_UNKNOWN;
    }
    new_lock->handle = handle;
    new_lock->identity = identity_copy;
    *lock = new_lock;
    return COM_UTIL_OK;
}

static int interprocess_lock_open_identity(const char *identity, com_util_interprocess_lock **lock)
{
    com_util_interprocess_lock *new_lock;
    HANDLE handle;
    char *identity_copy;

    if (identity == NULL || identity[0] == '\0' || lock == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    handle = CreateFileU(identity, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE)
    {
        return COM_UTIL_ERR_UNKNOWN;
    }
    identity_copy = dup_string(identity);
    if (identity_copy == NULL)
    {
        CloseHandle(handle);
        return COM_UTIL_ERR_UNKNOWN;
    }
    new_lock = (com_util_interprocess_lock *)com_util_calloc(1, sizeof(*new_lock));
    if (new_lock == NULL)
    {
        com_util_free(identity_copy);
        CloseHandle(handle);
        return COM_UTIL_ERR_UNKNOWN;
    }
    new_lock->handle = handle;
    new_lock->identity = identity_copy;
    *lock = new_lock;
    return COM_UTIL_OK;
}

static int app_lock_take(com_util_interprocess_rwlock *lock, DWORD flags, int timeout_ms)
{
    ULONGLONG deadline;
    OVERLAPPED ov = {0};

    if (lock == NULL || lock->locked)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    deadline = GetTickCount64() + (ULONGLONG)timeout_ms;
    do
    {
        if (LockFileEx(lock->handle, flags | LOCKFILE_FAIL_IMMEDIATELY, 0, INTERPROCESS_SYNC_RANGE_LOW,
                       INTERPROCESS_SYNC_RANGE_HIGH, &ov))
        {
            lock->locked = 1;
            return COM_UTIL_OK;
        }
        if (timeout_ms == COM_UTIL_SYNC_NO_WAIT)
        {
            return COM_UTIL_ERR_BUSY;
        }
        if (GetLastError() != ERROR_LOCK_VIOLATION)
        {
            return COM_UTIL_ERR_UNKNOWN;
        }
        Sleep(1);
    } while (timeout_ms == COM_UTIL_SYNC_WAIT_FOREVER || GetTickCount64() < deadline);

    return COM_UTIL_ERR_TIMEOUT;
}

static int interprocess_lock_take(com_util_interprocess_lock *lock, int timeout_ms)
{
    ULONGLONG deadline;
    OVERLAPPED ov = {0};

    if (lock == NULL || lock->locked)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    deadline = GetTickCount64() + (ULONGLONG)timeout_ms;
    do
    {
        if (LockFileEx(lock->handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0,
                       INTERPROCESS_SYNC_RANGE_LOW, INTERPROCESS_SYNC_RANGE_HIGH, &ov))
        {
            lock->locked = 1;
            return COM_UTIL_OK;
        }
        if (timeout_ms == COM_UTIL_SYNC_NO_WAIT)
        {
            return COM_UTIL_ERR_BUSY;
        }
        if (GetLastError() != ERROR_LOCK_VIOLATION)
        {
            return COM_UTIL_ERR_UNKNOWN;
        }
        Sleep(1);
    } while (timeout_ms == COM_UTIL_SYNC_WAIT_FOREVER || GetTickCount64() < deadline);

    return COM_UTIL_ERR_TIMEOUT;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_local_lock_create(com_util_local_lock **mtx)
{
    com_util_local_lock *new_mtx;

    if (mtx == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    new_mtx = (com_util_local_lock *)com_util_calloc(1, sizeof(*new_mtx));
    if (new_mtx == NULL)
    {
        return COM_UTIL_ERR_UNKNOWN;
    }
    InitializeSRWLock(&new_mtx->native);
    *mtx = new_mtx;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_local_lock_lock(com_util_local_lock *mtx, int timeout_ms)
{
    ULONGLONG deadline;

    if (mtx == NULL || timeout_ms < 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (timeout_ms == COM_UTIL_SYNC_WAIT_FOREVER)
    {
        AcquireSRWLockExclusive(&mtx->native);
        return COM_UTIL_OK;
    }
    deadline = GetTickCount64() + (ULONGLONG)timeout_ms;
    do
    {
        if (TryAcquireSRWLockExclusive(&mtx->native))
        {
            return COM_UTIL_OK;
        }
        if (timeout_ms == COM_UTIL_SYNC_NO_WAIT)
        {
            return COM_UTIL_ERR_BUSY;
        }
        Sleep(1);
    } while (GetTickCount64() < deadline);
    return COM_UTIL_ERR_TIMEOUT;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_local_lock_try_lock(com_util_local_lock *mtx)
{
    return com_util_local_lock_lock(mtx, COM_UTIL_SYNC_NO_WAIT);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_local_lock_unlock(com_util_local_lock *mtx)
{
    if (mtx == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    ReleaseSRWLockExclusive(&mtx->native);
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_local_lock_dispose(com_util_local_lock *mtx)
{
    com_util_free(mtx);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_condvar_create(com_util_condvar **cv)
{
    com_util_condvar *new_cv;

    if (cv == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    new_cv = (com_util_condvar *)com_util_calloc(1, sizeof(*new_cv));
    if (new_cv == NULL)
    {
        return COM_UTIL_ERR_UNKNOWN;
    }
    InitializeConditionVariable(&new_cv->native);
    *cv = new_cv;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_condvar_wait(com_util_condvar *cv, com_util_local_lock *mtx, int timeout_ms)
{
    DWORD wait_ms;

    if (cv == NULL || mtx == NULL || timeout_ms < 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
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
        return COM_UTIL_OK;
    }
    if (GetLastError() == ERROR_TIMEOUT)
    {
        return COM_UTIL_ERR_TIMEOUT;
    }
    else
    {
        return COM_UTIL_ERR_UNKNOWN;
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_condvar_signal(com_util_condvar *cv)
{
    if (cv == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    WakeConditionVariable(&cv->native);
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_condvar_broadcast(com_util_condvar *cv)
{
    if (cv == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    WakeAllConditionVariable(&cv->native);
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_condvar_dispose(com_util_condvar *cv)
{
    com_util_free(cv);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_local_rwlock_create(com_util_local_rwlock **rwlock)
{
    com_util_local_rwlock *new_lock;

    if (rwlock == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    new_lock = (com_util_local_rwlock *)com_util_calloc(1, sizeof(*new_lock));
    if (new_lock == NULL)
    {
        return COM_UTIL_ERR_UNKNOWN;
    }
    InitializeCriticalSection(&new_lock->mutex);
    InitializeConditionVariable(&new_lock->readers_cv);
    InitializeConditionVariable(&new_lock->writers_cv);
    *rwlock = new_lock;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_local_rwlock_lock_shared(com_util_local_rwlock *rwlock, int timeout_ms)
{
    DWORD wait_ms;

    if (rwlock == NULL || timeout_ms < 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
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
            return COM_UTIL_ERR_BUSY;
        }
        if (!SleepConditionVariableCS(&rwlock->readers_cv, &rwlock->mutex, wait_ms))
        {
            LeaveCriticalSection(&rwlock->mutex);
            if (GetLastError() == ERROR_TIMEOUT)
            {
                return COM_UTIL_ERR_TIMEOUT;
            }
            else
            {
                return COM_UTIL_ERR_UNKNOWN;
            }
        }
    }
    rwlock->active_readers++;
    LeaveCriticalSection(&rwlock->mutex);
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_local_rwlock_try_lock_shared(com_util_local_rwlock *rwlock)
{
    return com_util_local_rwlock_lock_shared(rwlock, COM_UTIL_SYNC_NO_WAIT);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_local_rwlock_lock_exclusive(com_util_local_rwlock *rwlock, int timeout_ms)
{
    DWORD wait_ms;

    if (rwlock == NULL || timeout_ms < 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
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
            return COM_UTIL_ERR_BUSY;
        }
        if (!SleepConditionVariableCS(&rwlock->writers_cv, &rwlock->mutex, wait_ms))
        {
            rwlock->waiting_writers--;
            LeaveCriticalSection(&rwlock->mutex);
            if (GetLastError() == ERROR_TIMEOUT)
            {
                return COM_UTIL_ERR_TIMEOUT;
            }
            else
            {
                return COM_UTIL_ERR_UNKNOWN;
            }
        }
    }
    rwlock->waiting_writers--;
    rwlock->writer_active = 1;
    LeaveCriticalSection(&rwlock->mutex);
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_local_rwlock_try_lock_exclusive(com_util_local_rwlock *rwlock)
{
    return com_util_local_rwlock_lock_exclusive(rwlock, COM_UTIL_SYNC_NO_WAIT);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_local_rwlock_unlock_shared(com_util_local_rwlock *rwlock)
{
    if (rwlock == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    EnterCriticalSection(&rwlock->mutex);
    if (rwlock->active_readers == 0)
    {
        LeaveCriticalSection(&rwlock->mutex);
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    rwlock->active_readers--;
    if (rwlock->active_readers == 0 && rwlock->waiting_writers > 0)
    {
        WakeConditionVariable(&rwlock->writers_cv);
    }
    LeaveCriticalSection(&rwlock->mutex);
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_local_rwlock_unlock_exclusive(com_util_local_rwlock *rwlock)
{
    if (rwlock == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    EnterCriticalSection(&rwlock->mutex);
    if (!rwlock->writer_active)
    {
        LeaveCriticalSection(&rwlock->mutex);
        return COM_UTIL_ERR_INVALID_ARGUMENT;
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
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_local_rwlock_dispose(com_util_local_rwlock *rwlock)
{
    if (rwlock != NULL)
    {
        DeleteCriticalSection(&rwlock->mutex);
        com_util_free(rwlock);
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_thread_create(com_util_thread **thread, com_util_thread_fn func, void *arg)
{
    struct com_util_thread_start_ctx *ctx;
    com_util_thread *new_thread;
    uintptr_t handle;

    if (thread == NULL || func == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    new_thread = (com_util_thread *)com_util_calloc(1, sizeof(*new_thread));
    ctx = (struct com_util_thread_start_ctx *)com_util_malloc(sizeof(*ctx));
    if (new_thread == NULL || ctx == NULL)
    {
        com_util_free(new_thread);
        com_util_free(ctx);
        return COM_UTIL_ERR_UNKNOWN;
    }
    ctx->func = func;
    ctx->arg = arg;
    handle = _beginthreadex(NULL, 0U, thread_start_proc, ctx, 0U, NULL);
    if (handle == 0U)
    {
        com_util_free(ctx);
        com_util_free(new_thread);
        return COM_UTIL_ERR_UNKNOWN;
    }
    new_thread->native = (HANDLE)handle;
    *thread = new_thread;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_thread_join(com_util_thread *thread, int timeout_ms)
{
    DWORD status;
    DWORD wait_ms;

    if (thread == NULL || timeout_ms < 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
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
        com_util_free(thread);
        return COM_UTIL_OK;
    }
    return wait_status_to_result(status);
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_thread_detach(com_util_thread *thread)
{
    if (thread != NULL)
    {
        CloseHandle(thread->native);
        com_util_free(thread);
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_interprocess_lock_open(const char *identity, com_util_interprocess_lock **lock)
{
    return interprocess_lock_open_identity(identity, lock);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_interprocess_lock_export_descriptor(const com_util_interprocess_lock *lock, void *descriptor,
                                                 size_t *descriptor_size)
{
    if (lock == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    return interprocess_sync_descriptor_export(lock->identity, INTERPROCESS_SYNC_KIND_LOCK,
                                               (uint8_t)COM_UTIL_INTERPROCESS_SYNC_BACKEND_LOCK_FILE, descriptor,
                                               descriptor_size);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_interprocess_lock_import_descriptor(const void *descriptor, size_t descriptor_size,
                                                 com_util_interprocess_lock **lock)
{
    char *identity;
    int result;

    if (lock == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    result = interprocess_sync_descriptor_import(descriptor, descriptor_size, INTERPROCESS_SYNC_KIND_LOCK,
                                                 (uint8_t)COM_UTIL_INTERPROCESS_SYNC_BACKEND_LOCK_FILE, &identity);
    if (result != COM_UTIL_OK)
    {
        return result;
    }
    result = interprocess_lock_open_identity(identity, lock);
    com_util_free(identity);
    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_interprocess_lock_lock(com_util_interprocess_lock *lock, int timeout_ms)
{
    if (timeout_ms < 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    return interprocess_lock_take(lock, timeout_ms);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_interprocess_lock_try_lock(com_util_interprocess_lock *lock)
{
    return com_util_interprocess_lock_lock(lock, COM_UTIL_SYNC_NO_WAIT);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_interprocess_lock_unlock(com_util_interprocess_lock *lock)
{
    OVERLAPPED ov = {0};

    if (lock == NULL || !lock->locked)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (!UnlockFileEx(lock->handle, 0, INTERPROCESS_SYNC_RANGE_LOW, INTERPROCESS_SYNC_RANGE_HIGH, &ov))
    {
        return COM_UTIL_ERR_UNKNOWN;
    }
    lock->locked = 0;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_interprocess_lock_dispose(com_util_interprocess_lock *lock)
{
    if (lock != NULL)
    {
        if (lock->locked)
        {
            (void)com_util_interprocess_lock_unlock(lock);
        }
        CloseHandle(lock->handle);
        com_util_free(lock->identity);
        com_util_free(lock);
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_interprocess_rwlock_open(const char *identity, com_util_interprocess_rwlock **lock)
{
    return app_lock_open_identity(identity, lock);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_interprocess_rwlock_export_descriptor(const com_util_interprocess_rwlock *lock, void *descriptor,
                                                   size_t *descriptor_size)
{
    if (lock == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    return interprocess_sync_descriptor_export(lock->identity, INTERPROCESS_SYNC_KIND_RWLOCK,
                                               (uint8_t)COM_UTIL_INTERPROCESS_SYNC_BACKEND_LOCK_FILE, descriptor,
                                               descriptor_size);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_interprocess_rwlock_import_descriptor(const void *descriptor, size_t descriptor_size,
                                                   com_util_interprocess_rwlock **lock)
{
    char *identity;
    int result;

    if (lock == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    result = interprocess_sync_descriptor_import(descriptor, descriptor_size, INTERPROCESS_SYNC_KIND_RWLOCK,
                                                 (uint8_t)COM_UTIL_INTERPROCESS_SYNC_BACKEND_LOCK_FILE, &identity);
    if (result != COM_UTIL_OK)
    {
        return result;
    }
    result = app_lock_open_identity(identity, lock);
    com_util_free(identity);
    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_interprocess_rwlock_lock_shared(com_util_interprocess_rwlock *lock, int timeout_ms)
{
    if (timeout_ms < 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    return app_lock_take(lock, 0U, timeout_ms);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_interprocess_rwlock_try_lock_shared(com_util_interprocess_rwlock *lock)
{
    return com_util_interprocess_rwlock_lock_shared(lock, COM_UTIL_SYNC_NO_WAIT);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_interprocess_rwlock_lock_exclusive(com_util_interprocess_rwlock *lock, int timeout_ms)
{
    if (timeout_ms < 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    return app_lock_take(lock, LOCKFILE_EXCLUSIVE_LOCK, timeout_ms);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_interprocess_rwlock_try_lock_exclusive(com_util_interprocess_rwlock *lock)
{
    return com_util_interprocess_rwlock_lock_exclusive(lock, COM_UTIL_SYNC_NO_WAIT);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_interprocess_rwlock_unlock(com_util_interprocess_rwlock *lock)
{
    OVERLAPPED ov = {0};

    if (lock == NULL || !lock->locked)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (!UnlockFileEx(lock->handle, 0, INTERPROCESS_SYNC_RANGE_LOW, INTERPROCESS_SYNC_RANGE_HIGH, &ov))
    {
        return COM_UTIL_ERR_UNKNOWN;
    }
    lock->locked = 0;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_interprocess_rwlock_dispose(com_util_interprocess_rwlock *lock)
{
    if (lock != NULL)
    {
        if (lock->locked)
        {
            (void)com_util_interprocess_rwlock_unlock(lock);
        }
        CloseHandle(lock->handle);
        com_util_free(lock->identity);
        com_util_free(lock);
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

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

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_sleep_ms(int ms)
{
    if (ms <= 0)
    {
        return;
    }
    Sleep((DWORD)ms);
}

#endif /* PLATFORM_WINDOWS */
