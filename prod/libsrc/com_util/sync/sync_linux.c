/**
 *******************************************************************************
 *  @file           sync_linux.c
 *  @brief          Linux 向けスレッド・同期プリミティブ実装。
 *  @author         Tetsuo Honda
 *  @date           2026/04/20
 *  @version        1.0.0
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)

    #include <errno.h>
    #include <sched.h>
    #include <stdlib.h>
    #include <string.h>
    #include <time.h>

    #include <com_util/sync/sync.h>

struct com_util_thread_start_ctx
{
    com_util_thread_func_t func;
    void                  *arg;
};

static void add_timeout_ms(struct timespec *abs_ts, uint32_t timeout_ms)
{
    clock_gettime(CLOCK_REALTIME, abs_ts);
    abs_ts->tv_sec += (time_t)(timeout_ms / 1000U);
    abs_ts->tv_nsec += (long)((timeout_ms % 1000U) * 1000000UL);
    if (abs_ts->tv_nsec >= 1000000000L)
    {
        abs_ts->tv_sec++;
        abs_ts->tv_nsec -= 1000000000L;
    }
}

static void clear_thread_handle(com_util_thread_t *thread)
{
    memset(thread, 0, sizeof(*thread));
}

static void *thread_start_proc(void *opaque)
{
    struct com_util_thread_start_ctx *ctx = (struct com_util_thread_start_ctx *)opaque;
    com_util_thread_func_t            func;
    void                             *arg;

    func = ctx->func;
    arg = ctx->arg;
    free(ctx);

    func(arg);
    return NULL;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_mutex_init(com_util_mutex_t *mtx)
{
    return pthread_mutex_init(mtx, NULL);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_mutex_lock(com_util_mutex_t *mtx)
{
    return pthread_mutex_lock(mtx);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_mutex_timedlock(com_util_mutex_t *mtx, uint32_t timeout_ms)
{
    struct timespec abs_ts;

    add_timeout_ms(&abs_ts, timeout_ms);
    return pthread_mutex_timedlock(mtx, &abs_ts);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_mutex_unlock(com_util_mutex_t *mtx)
{
    return pthread_mutex_unlock(mtx);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_mutex_destroy(com_util_mutex_t *mtx)
{
    return pthread_mutex_destroy(mtx);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_condvar_init(com_util_condvar_t *cv)
{
    return pthread_cond_init(cv, NULL);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_condvar_wait(com_util_condvar_t *cv, com_util_mutex_t *mtx)
{
    return pthread_cond_wait(cv, mtx);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_condvar_timedwait(com_util_condvar_t *cv, com_util_mutex_t *mtx,
                               uint32_t timeout_ms)
{
    struct timespec abs_ts;

    add_timeout_ms(&abs_ts, timeout_ms);
    return pthread_cond_timedwait(cv, mtx, &abs_ts);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_condvar_signal(com_util_condvar_t *cv)
{
    return pthread_cond_signal(cv);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_condvar_broadcast(com_util_condvar_t *cv)
{
    return pthread_cond_broadcast(cv);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_condvar_destroy(com_util_condvar_t *cv)
{
    return pthread_cond_destroy(cv);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_rwlock_init(com_util_rwlock_t *rwlock)
{
    return pthread_rwlock_init(rwlock, NULL);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_rwlock_lock_shared(com_util_rwlock_t *rwlock)
{
    return pthread_rwlock_rdlock(rwlock);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_rwlock_timedlock_shared(com_util_rwlock_t *rwlock, uint32_t timeout_ms)
{
    struct timespec abs_ts;

    add_timeout_ms(&abs_ts, timeout_ms);
    return pthread_rwlock_timedrdlock(rwlock, &abs_ts);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_rwlock_lock_exclusive(com_util_rwlock_t *rwlock)
{
    return pthread_rwlock_wrlock(rwlock);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_rwlock_unlock_shared(com_util_rwlock_t *rwlock)
{
    return pthread_rwlock_unlock(rwlock);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_rwlock_unlock_exclusive(com_util_rwlock_t *rwlock)
{
    return pthread_rwlock_unlock(rwlock);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_rwlock_destroy(com_util_rwlock_t *rwlock)
{
    return pthread_rwlock_destroy(rwlock);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_thread_create(com_util_thread_t *thread,
                           com_util_thread_func_t func, void *arg)
{
    struct com_util_thread_start_ctx *ctx;
    int                               rc;

    ctx = (struct com_util_thread_start_ctx *)malloc(sizeof(*ctx));
    if (ctx == NULL)
    {
        return -1;
    }

    ctx->func = func;
    ctx->arg = arg;
    rc = pthread_create(thread, NULL, thread_start_proc, ctx);
    if (rc != 0)
    {
        free(ctx);
    }

    return rc;
}

/* doxygen コメントは、ヘッダに記載 */
void com_util_thread_join(com_util_thread_t *thread)
{
    pthread_join(*thread, NULL);
    clear_thread_handle(thread);
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_thread_join_timed(com_util_thread_t *thread, uint32_t timeout_ms)
{
    struct timespec abs_ts;
    int             rc;

    add_timeout_ms(&abs_ts, timeout_ms);
    rc = pthread_timedjoin_np(*thread, NULL, &abs_ts);
    if (rc == 0)
    {
        clear_thread_handle(thread);
        return 0;
    }
    if (rc == ETIMEDOUT)
    {
        return 1;
    }
    return -1;
}

/* doxygen コメントは、ヘッダに記載 */
int com_util_thread_detach(com_util_thread_t *thread)
{
    int rc;

    rc = pthread_detach(*thread);
    if (rc == 0)
    {
        clear_thread_handle(thread);
    }
    return rc;
}

/* doxygen コメントは、ヘッダに記載 */
void com_util_call_once(com_util_once_flag_t *flag, com_util_once_func_t func)
{
    int32_t expected = 0;

    if (__atomic_compare_exchange_n(&flag->state, &expected, 1, 0,
                                    __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
    {
        func();
        __atomic_store_n(&flag->state, 2, __ATOMIC_RELEASE);
        return;
    }

    while (__atomic_load_n(&flag->state, __ATOMIC_ACQUIRE) != 2)
    {
        sched_yield();
    }
}

#elif defined(PLATFORM_WINDOWS) && defined(COMPILER_MSVC)
    #pragma warning(disable : 4206)
#endif /* PLATFORM_ */
