/**
 *******************************************************************************
 *  @file           sync_windows.c
 *  @brief          Windows 向けスレッド・同期プリミティブ実装。
 *  @author         Tetsuo Honda
 *  @date           2026/04/20
 *  @version        1.0.0
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)

    #include <process.h>
    #include <stdlib.h>

    #include <com_util/sync/sync.h>

struct com_util_thread_start_ctx
{
    com_util_thread_func_t func;
    void                  *arg;
};

static unsigned __stdcall thread_start_proc(void *opaque)
{
    struct com_util_thread_start_ctx *ctx = (struct com_util_thread_start_ctx *)opaque;
    com_util_thread_func_t            func;
    void                             *arg;

    func = ctx->func;
    arg = ctx->arg;
    free(ctx);

    func(arg);
    return 0U;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_mutex_init(com_util_mutex_t *mtx)
{
    return InitializeCriticalSectionAndSpinCount(mtx, 1000) ? 0 : -1;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_mutex_lock(com_util_mutex_t *mtx)
{
    EnterCriticalSection(mtx);
    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_mutex_timedlock(com_util_mutex_t *mtx, uint32_t timeout_ms)
{
    ULONGLONG deadline = GetTickCount64() + (ULONGLONG)timeout_ms;

    while (!TryEnterCriticalSection(mtx))
    {
        if (GetTickCount64() >= deadline)
        {
            return -1;
        }
        SwitchToThread();
    }

    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_mutex_unlock(com_util_mutex_t *mtx)
{
    LeaveCriticalSection(mtx);
    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_mutex_destroy(com_util_mutex_t *mtx)
{
    DeleteCriticalSection(mtx);
    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_condvar_init(com_util_condvar_t *cv)
{
    InitializeConditionVariable(cv);
    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_condvar_wait(com_util_condvar_t *cv, com_util_mutex_t *mtx)
{
    return SleepConditionVariableCS(cv, mtx, INFINITE) ? 0 : -1;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_condvar_timedwait(com_util_condvar_t *cv, com_util_mutex_t *mtx,
                               uint32_t timeout_ms)
{
    return SleepConditionVariableCS(cv, mtx, (DWORD)timeout_ms) ? 0 : -1;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_condvar_signal(com_util_condvar_t *cv)
{
    WakeConditionVariable(cv);
    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_condvar_broadcast(com_util_condvar_t *cv)
{
    WakeAllConditionVariable(cv);
    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_condvar_destroy(com_util_condvar_t *cv)
{
    (void)cv;
    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_rwlock_init(com_util_rwlock_t *rwlock)
{
    InitializeSRWLock(rwlock);
    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_rwlock_lock_shared(com_util_rwlock_t *rwlock)
{
    AcquireSRWLockShared(rwlock);
    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_rwlock_timedlock_shared(com_util_rwlock_t *rwlock, uint32_t timeout_ms)
{
    ULONGLONG deadline = GetTickCount64() + (ULONGLONG)timeout_ms;

    while (!TryAcquireSRWLockShared(rwlock))
    {
        if (GetTickCount64() >= deadline)
        {
            return -1;
        }
        SwitchToThread();
    }
    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_rwlock_lock_exclusive(com_util_rwlock_t *rwlock)
{
    AcquireSRWLockExclusive(rwlock);
    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_rwlock_unlock_shared(com_util_rwlock_t *rwlock)
{
    ReleaseSRWLockShared(rwlock);
    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_rwlock_unlock_exclusive(com_util_rwlock_t *rwlock)
{
    ReleaseSRWLockExclusive(rwlock);
    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_rwlock_destroy(com_util_rwlock_t *rwlock)
{
    (void)rwlock;
    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_thread_create(com_util_thread_t *thread,
                           com_util_thread_func_t func, void *arg)
{
    struct com_util_thread_start_ctx *ctx;
    uintptr_t                         handle;

    ctx = (struct com_util_thread_start_ctx *)malloc(sizeof(*ctx));
    if (ctx == NULL)
    {
        return -1;
    }

    ctx->func = func;
    ctx->arg = arg;
    handle = _beginthreadex(NULL, 0U, thread_start_proc, ctx, 0U, NULL);
    if (handle == 0U)
    {
        free(ctx);
        return -1;
    }

    *thread = (HANDLE)handle;
    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
void com_util_thread_join(com_util_thread_t *thread)
{
    if (*thread != NULL)
    {
        WaitForSingleObject(*thread, INFINITE);
        CloseHandle(*thread);
        *thread = NULL;
    }
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_thread_join_timed(com_util_thread_t *thread, uint32_t timeout_ms)
{
    DWORD status;

    if (*thread == NULL)
    {
        return -1;
    }

    status = WaitForSingleObject(*thread, (DWORD)timeout_ms);
    if (status == WAIT_OBJECT_0)
    {
        CloseHandle(*thread);
        *thread = NULL;
        return 0;
    }
    if (status == WAIT_TIMEOUT)
    {
        return 1;
    }
    return -1;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_thread_detach(com_util_thread_t *thread)
{
    if (*thread == NULL)
    {
        return -1;
    }

    CloseHandle(*thread);
    *thread = NULL;
    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
void com_util_call_once(com_util_once_flag_t *flag, com_util_once_func_t func)
{
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

#endif /* PLATFORM_WINDOWS */
