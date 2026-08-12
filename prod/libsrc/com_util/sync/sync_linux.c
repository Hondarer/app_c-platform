/**
 *******************************************************************************
 *  @file           sync_linux.c
 *  @brief          Linux 向けのスレッドと同期プリミティブを実装します。
 *******************************************************************************
 */

#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif /* _GNU_SOURCE */

#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)

    #include <errno.h>
    #include <fcntl.h>
    #include <pthread.h>
    #include <sched.h>
    #include <stdlib.h>
    #include <string.h>
    #include <sys/file.h>
    #include <time.h>
    #include <unistd.h>

    #include <com_util/sync/sync.h>
    #include <com_util/sync/sync_descriptor.h>

struct com_util_local_lock
{
    pthread_mutex_t native;
};

struct com_util_condvar
{
    pthread_cond_t native;
};

struct com_util_local_rwlock
{
    pthread_mutex_t mutex;
    pthread_cond_t readers_cv;
    pthread_cond_t writers_cv;
    unsigned int active_readers;
    unsigned int waiting_writers;
    int writer_active;
    int _pad_struct_end;
};

struct com_util_thread
{
    pthread_t native;
};

struct com_util_interprocess_lock
{
    char *identity;
    int fd;
    int locked;
};

struct com_util_interprocess_rwlock
{
    char *identity;
    int fd;
    int locked;
};

struct com_util_thread_start_ctx
{
    com_util_thread_fn func;
    void *arg;
};

static uint64_t monotonic_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000U) + ((uint64_t)ts.tv_nsec / 1000000U);
}

static void monotonic_deadline(struct timespec *abs_ts, int timeout_ms)
{
    /* 呼び出し元で負値チェック済みのため、unsigned int として演算する。 */
    unsigned int ms = (unsigned int)timeout_ms;

    clock_gettime(CLOCK_MONOTONIC, abs_ts);
    abs_ts->tv_sec += (time_t)(ms / 1000U);
    abs_ts->tv_nsec += (long)((ms % 1000U) * 1000000UL);
    if (abs_ts->tv_nsec >= 1000000000L)
    {
        abs_ts->tv_sec++;
        abs_ts->tv_nsec -= 1000000000L;
    }
}

static int cond_init_monotonic(pthread_cond_t *cond)
{
    pthread_condattr_t attr;
    int rc;

    rc = pthread_condattr_init(&attr);
    if (rc != 0)
    {
        return rc;
    }
    rc = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    if (rc == 0)
    {
        rc = pthread_cond_init(cond, &attr);
    }
    pthread_condattr_destroy(&attr);
    return rc;
}

static int map_wait_rc(int rc)
{
    if (rc == 0)
    {
        return COM_UTIL_OK;
    }
    if (rc == ETIMEDOUT)
    {
        return COM_UTIL_ERR_TIMEOUT;
    }
    if (rc == EBUSY || rc == EAGAIN || rc == EACCES)
    {
        return COM_UTIL_ERR_BUSY;
    }
    return COM_UTIL_ERR_UNKNOWN;
}

static void *thread_start_proc(void *opaque)
{
    struct com_util_thread_start_ctx *ctx = (struct com_util_thread_start_ctx *)opaque;
    com_util_thread_fn func = ctx->func;
    void *arg = ctx->arg;

    free(ctx);
    func(arg);
    return NULL;
}

static int app_lock_open_identity(const char *identity, com_util_interprocess_rwlock **lock)
{
    com_util_interprocess_rwlock *new_lock;
    int fd;
    char *identity_copy;

    if (identity == NULL || identity[0] == '\0' || lock == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    fd = open(identity, O_RDWR | O_CREAT | O_CLOEXEC, 0666);
    if (fd < 0)
    {
        return COM_UTIL_ERR_UNKNOWN;
    }

    identity_copy = strdup(identity);
    if (identity_copy == NULL)
    {
        close(fd);
        return COM_UTIL_ERR_UNKNOWN;
    }

    new_lock = (com_util_interprocess_rwlock *)calloc(1, sizeof(*new_lock));
    if (new_lock == NULL)
    {
        free(identity_copy);
        close(fd);
        return COM_UTIL_ERR_UNKNOWN;
    }

    new_lock->fd = fd;
    new_lock->identity = identity_copy;
    *lock = new_lock;
    return COM_UTIL_OK;
}

static int interprocess_lock_open_identity(const char *identity, com_util_interprocess_lock **lock)
{
    com_util_interprocess_lock *new_lock;
    int fd;
    char *identity_copy;

    if (identity == NULL || identity[0] == '\0' || lock == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    fd = open(identity, O_RDWR | O_CREAT | O_CLOEXEC, 0666);
    if (fd < 0)
    {
        return COM_UTIL_ERR_UNKNOWN;
    }

    identity_copy = strdup(identity);
    if (identity_copy == NULL)
    {
        close(fd);
        return COM_UTIL_ERR_UNKNOWN;
    }

    new_lock = (com_util_interprocess_lock *)calloc(1, sizeof(*new_lock));
    if (new_lock == NULL)
    {
        free(identity_copy);
        close(fd);
        return COM_UTIL_ERR_UNKNOWN;
    }

    new_lock->fd = fd;
    new_lock->identity = identity_copy;
    *lock = new_lock;
    return COM_UTIL_OK;
}

static int app_lock_take(com_util_interprocess_rwlock *lock, int operation, int timeout_ms)
{
    uint64_t deadline;

    if (lock == NULL || lock->locked)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    if (timeout_ms == COM_UTIL_SYNC_WAIT_FOREVER)
    {
        while (flock(lock->fd, operation) != 0)
        {
            if (errno != EINTR)
            {
                return COM_UTIL_ERR_UNKNOWN;
            }
        }
        lock->locked = 1;
        return COM_UTIL_OK;
    }

    /* 呼び出し元で timeout_ms < 0 を拒否済み。WAIT_FOREVER 分岐後は 0 以上の有限値 */
    deadline = monotonic_ms() + (uint64_t)timeout_ms;
    do
    {
        if (flock(lock->fd, operation | LOCK_NB) == 0)
        {
            lock->locked = 1;
            return COM_UTIL_OK;
        }
        if (errno != EWOULDBLOCK && errno != EINTR)
        {
            return COM_UTIL_ERR_UNKNOWN;
        }
        if (timeout_ms == COM_UTIL_SYNC_NO_WAIT)
        {
            return COM_UTIL_ERR_BUSY;
        }
        {
            struct timespec sleep_ts = {0, 1000000L};
            nanosleep(&sleep_ts, NULL);
        }
    } while (monotonic_ms() < deadline);

    return COM_UTIL_ERR_TIMEOUT;
}

static int interprocess_lock_take(com_util_interprocess_lock *lock, int timeout_ms)
{
    uint64_t deadline;

    if (lock == NULL || lock->locked)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    if (timeout_ms == COM_UTIL_SYNC_WAIT_FOREVER)
    {
        while (flock(lock->fd, LOCK_EX) != 0)
        {
            if (errno != EINTR)
            {
                return COM_UTIL_ERR_UNKNOWN;
            }
        }
        lock->locked = 1;
        return COM_UTIL_OK;
    }

    /* 呼び出し元で timeout_ms < 0 を拒否済み。WAIT_FOREVER 分岐後は 0 以上の有限値 */
    deadline = monotonic_ms() + (uint64_t)timeout_ms;
    do
    {
        if (flock(lock->fd, LOCK_EX | LOCK_NB) == 0)
        {
            lock->locked = 1;
            return COM_UTIL_OK;
        }
        if (errno != EWOULDBLOCK && errno != EINTR)
        {
            return COM_UTIL_ERR_UNKNOWN;
        }
        if (timeout_ms == COM_UTIL_SYNC_NO_WAIT)
        {
            return COM_UTIL_ERR_BUSY;
        }
        {
            struct timespec sleep_ts = {0, 1000000L};
            nanosleep(&sleep_ts, NULL);
        }
    } while (monotonic_ms() < deadline);

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
    new_mtx = (com_util_local_lock *)calloc(1, sizeof(*new_mtx));
    if (new_mtx == NULL)
    {
        return COM_UTIL_ERR_UNKNOWN;
    }
    if (pthread_mutex_init(&new_mtx->native, NULL) != 0)
    {
        free(new_mtx);
        return COM_UTIL_ERR_UNKNOWN;
    }
    *mtx = new_mtx;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_local_lock_lock(com_util_local_lock *mtx, int timeout_ms)
{
    uint64_t deadline;

    if (mtx == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (timeout_ms < 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (timeout_ms == COM_UTIL_SYNC_WAIT_FOREVER)
    {
        return map_wait_rc(pthread_mutex_lock(&mtx->native));
    }
    if (timeout_ms == COM_UTIL_SYNC_NO_WAIT)
    {
        return map_wait_rc(pthread_mutex_trylock(&mtx->native));
    }
    /* 上で timeout_ms < 0 を拒否済み。WAIT_FOREVER 分岐後は 0 以上の有限値 */
    deadline = monotonic_ms() + (uint64_t)timeout_ms;
    do
    {
        int rc = pthread_mutex_trylock(&mtx->native);
        if (rc == 0)
        {
            return COM_UTIL_OK;
        }
        if (rc != EBUSY)
        {
            return COM_UTIL_ERR_UNKNOWN;
        }
        {
            struct timespec sleep_ts = {0, 1000000L};
            nanosleep(&sleep_ts, NULL);
        }
    } while (monotonic_ms() < deadline);
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
    return map_wait_rc(pthread_mutex_unlock(&mtx->native));
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_local_lock_destroy(com_util_local_lock *mtx)
{
    if (mtx != NULL)
    {
        pthread_mutex_destroy(&mtx->native);
        free(mtx);
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_condvar_create(com_util_condvar **cv)
{
    com_util_condvar *new_cv;

    if (cv == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    new_cv = (com_util_condvar *)calloc(1, sizeof(*new_cv));
    if (new_cv == NULL)
    {
        return COM_UTIL_ERR_UNKNOWN;
    }
    if (cond_init_monotonic(&new_cv->native) != 0)
    {
        free(new_cv);
        return COM_UTIL_ERR_UNKNOWN;
    }
    *cv = new_cv;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_condvar_wait(com_util_condvar *cv, com_util_local_lock *mtx, int timeout_ms)
{
    struct timespec abs_ts;

    if (cv == NULL || mtx == NULL || timeout_ms < 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (timeout_ms == COM_UTIL_SYNC_WAIT_FOREVER)
    {
        return map_wait_rc(pthread_cond_wait(&cv->native, &mtx->native));
    }
    monotonic_deadline(&abs_ts, timeout_ms);
    return map_wait_rc(pthread_cond_timedwait(&cv->native, &mtx->native, &abs_ts));
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_condvar_signal(com_util_condvar *cv)
{
    if (cv == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    return map_wait_rc(pthread_cond_signal(&cv->native));
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_condvar_broadcast(com_util_condvar *cv)
{
    if (cv == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    return map_wait_rc(pthread_cond_broadcast(&cv->native));
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_condvar_destroy(com_util_condvar *cv)
{
    if (cv != NULL)
    {
        pthread_cond_destroy(&cv->native);
        free(cv);
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_local_rwlock_create(com_util_local_rwlock **rwlock)
{
    com_util_local_rwlock *new_lock;

    if (rwlock == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    new_lock = (com_util_local_rwlock *)calloc(1, sizeof(*new_lock));
    if (new_lock == NULL)
    {
        return COM_UTIL_ERR_UNKNOWN;
    }
    if (pthread_mutex_init(&new_lock->mutex, NULL) != 0 || cond_init_monotonic(&new_lock->readers_cv) != 0 ||
        cond_init_monotonic(&new_lock->writers_cv) != 0)
    {
        pthread_mutex_destroy(&new_lock->mutex);
        pthread_cond_destroy(&new_lock->readers_cv);
        pthread_cond_destroy(&new_lock->writers_cv);
        free(new_lock);
        return COM_UTIL_ERR_UNKNOWN;
    }
    *rwlock = new_lock;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_local_rwlock_lock_shared(com_util_local_rwlock *rwlock, int timeout_ms)
{
    struct timespec abs_ts;
    int rc = 0;

    if (rwlock == NULL || timeout_ms < 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    pthread_mutex_lock(&rwlock->mutex);
    if (timeout_ms != COM_UTIL_SYNC_WAIT_FOREVER)
    {
        monotonic_deadline(&abs_ts, timeout_ms);
    }
    while (rwlock->writer_active || rwlock->waiting_writers > 0)
    {
        if (timeout_ms == COM_UTIL_SYNC_NO_WAIT)
        {
            pthread_mutex_unlock(&rwlock->mutex);
            return COM_UTIL_ERR_BUSY;
        }
        if (timeout_ms == COM_UTIL_SYNC_WAIT_FOREVER)
        {
            rc = pthread_cond_wait(&rwlock->readers_cv, &rwlock->mutex);
        }
        else
        {
            rc = pthread_cond_timedwait(&rwlock->readers_cv, &rwlock->mutex, &abs_ts);
        }
        if (rc == ETIMEDOUT)
        {
            pthread_mutex_unlock(&rwlock->mutex);
            return COM_UTIL_ERR_TIMEOUT;
        }
        if (rc != 0)
        {
            pthread_mutex_unlock(&rwlock->mutex);
            return COM_UTIL_ERR_UNKNOWN;
        }
    }
    rwlock->active_readers++;
    pthread_mutex_unlock(&rwlock->mutex);
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
    struct timespec abs_ts;
    int rc = 0;

    if (rwlock == NULL || timeout_ms < 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    pthread_mutex_lock(&rwlock->mutex);
    rwlock->waiting_writers++;
    if (timeout_ms != COM_UTIL_SYNC_WAIT_FOREVER)
    {
        monotonic_deadline(&abs_ts, timeout_ms);
    }
    while (rwlock->writer_active || rwlock->active_readers > 0)
    {
        if (timeout_ms == COM_UTIL_SYNC_NO_WAIT)
        {
            rwlock->waiting_writers--;
            pthread_mutex_unlock(&rwlock->mutex);
            return COM_UTIL_ERR_BUSY;
        }
        if (timeout_ms == COM_UTIL_SYNC_WAIT_FOREVER)
        {
            rc = pthread_cond_wait(&rwlock->writers_cv, &rwlock->mutex);
        }
        else
        {
            rc = pthread_cond_timedwait(&rwlock->writers_cv, &rwlock->mutex, &abs_ts);
        }
        if (rc == ETIMEDOUT)
        {
            rwlock->waiting_writers--;
            pthread_mutex_unlock(&rwlock->mutex);
            return COM_UTIL_ERR_TIMEOUT;
        }
        if (rc != 0)
        {
            rwlock->waiting_writers--;
            pthread_mutex_unlock(&rwlock->mutex);
            return COM_UTIL_ERR_UNKNOWN;
        }
    }
    rwlock->waiting_writers--;
    rwlock->writer_active = 1;
    pthread_mutex_unlock(&rwlock->mutex);
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
    pthread_mutex_lock(&rwlock->mutex);
    if (rwlock->active_readers == 0)
    {
        pthread_mutex_unlock(&rwlock->mutex);
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    rwlock->active_readers--;
    if (rwlock->active_readers == 0 && rwlock->waiting_writers > 0)
    {
        pthread_cond_signal(&rwlock->writers_cv);
    }
    pthread_mutex_unlock(&rwlock->mutex);
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_local_rwlock_unlock_exclusive(com_util_local_rwlock *rwlock)
{
    if (rwlock == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    pthread_mutex_lock(&rwlock->mutex);
    if (!rwlock->writer_active)
    {
        pthread_mutex_unlock(&rwlock->mutex);
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    rwlock->writer_active = 0;
    if (rwlock->waiting_writers > 0)
    {
        pthread_cond_signal(&rwlock->writers_cv);
    }
    else
    {
        pthread_cond_broadcast(&rwlock->readers_cv);
    }
    pthread_mutex_unlock(&rwlock->mutex);
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_local_rwlock_destroy(com_util_local_rwlock *rwlock)
{
    if (rwlock != NULL)
    {
        pthread_mutex_destroy(&rwlock->mutex);
        pthread_cond_destroy(&rwlock->readers_cv);
        pthread_cond_destroy(&rwlock->writers_cv);
        free(rwlock);
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_thread_create(com_util_thread **thread, com_util_thread_fn func, void *arg)
{
    struct com_util_thread_start_ctx *ctx;
    com_util_thread *new_thread;
    int rc;

    if (thread == NULL || func == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    new_thread = (com_util_thread *)calloc(1, sizeof(*new_thread));
    ctx = (struct com_util_thread_start_ctx *)malloc(sizeof(*ctx));
    if (new_thread == NULL || ctx == NULL)
    {
        free(new_thread);
        free(ctx);
        return COM_UTIL_ERR_UNKNOWN;
    }
    ctx->func = func;
    ctx->arg = arg;
    rc = pthread_create(&new_thread->native, NULL, thread_start_proc, ctx);
    if (rc != 0)
    {
        free(ctx);
        free(new_thread);
        return COM_UTIL_ERR_UNKNOWN;
    }
    *thread = new_thread;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_thread_join(com_util_thread *thread, int timeout_ms)
{
    struct timespec abs_ts;
    int rc;

    if (thread == NULL || timeout_ms < 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (timeout_ms == COM_UTIL_SYNC_WAIT_FOREVER)
    {
        rc = pthread_join(thread->native, NULL);
    }
    else
    {
        /* 上で timeout_ms < 0 を拒否済み。有限タイムアウトは 0 以上 */
        uint64_t deadline = monotonic_ms() + (uint64_t)timeout_ms;
        do
        {
            rc = pthread_tryjoin_np(thread->native, NULL);
            if (rc == 0)
            {
                break;
            }
            if (rc != EBUSY)
            {
                break;
            }
            if (timeout_ms == COM_UTIL_SYNC_NO_WAIT)
            {
                break;
            }
            abs_ts.tv_sec = 0;
            abs_ts.tv_nsec = 1000000L;
            nanosleep(&abs_ts, NULL);
        } while (monotonic_ms() < deadline);
        if (rc == EBUSY)
        {
            rc = ETIMEDOUT;
        }
    }
    if (rc == ETIMEDOUT)
    {
        return COM_UTIL_ERR_TIMEOUT;
    }
    if (rc != 0)
    {
        return COM_UTIL_ERR_UNKNOWN;
    }
    free(thread);
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_thread_detach(com_util_thread *thread)
{
    if (thread != NULL)
    {
        pthread_detach(thread->native);
        free(thread);
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
    free(identity);
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
    if (lock == NULL || !lock->locked)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (flock(lock->fd, LOCK_UN) != 0)
    {
        return COM_UTIL_ERR_UNKNOWN;
    }
    lock->locked = 0;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_interprocess_lock_destroy(com_util_interprocess_lock *lock)
{
    if (lock != NULL)
    {
        if (lock->locked)
        {
            (void)com_util_interprocess_lock_unlock(lock);
        }
        close(lock->fd);
        free(lock->identity);
        free(lock);
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
    free(identity);
    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_interprocess_rwlock_lock_shared(com_util_interprocess_rwlock *lock, int timeout_ms)
{
    if (timeout_ms < 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    return app_lock_take(lock, LOCK_SH, timeout_ms);
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
    return app_lock_take(lock, LOCK_EX, timeout_ms);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_interprocess_rwlock_try_lock_exclusive(com_util_interprocess_rwlock *lock)
{
    return com_util_interprocess_rwlock_lock_exclusive(lock, COM_UTIL_SYNC_NO_WAIT);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_interprocess_rwlock_unlock(com_util_interprocess_rwlock *lock)
{
    if (lock == NULL || !lock->locked)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (flock(lock->fd, LOCK_UN) != 0)
    {
        return COM_UTIL_ERR_UNKNOWN;
    }
    lock->locked = 0;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_interprocess_rwlock_destroy(com_util_interprocess_rwlock *lock)
{
    if (lock != NULL)
    {
        if (lock->locked)
        {
            (void)com_util_interprocess_rwlock_unlock(lock);
        }
        close(lock->fd);
        free(lock->identity);
        free(lock);
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_call_once(com_util_once_flag *flag, void (*func)(void))
{
    int32_t expected = 0;

    if (flag == NULL || func == NULL)
    {
        return;
    }
    if (__atomic_compare_exchange_n(&flag->state, &expected, 1, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
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

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_sleep_ms(int ms)
{
    struct timespec req;
    struct timespec rem;
    unsigned int ums;

    if (ms <= 0)
    {
        return;
    }
    ums = (unsigned int)ms;
    req.tv_sec = (time_t)(ums / 1000U);
    req.tv_nsec = (long)((ums % 1000U) * 1000000UL);
    while (nanosleep(&req, &rem) == -1 && errno == EINTR)
    {
        req = rem;
    }
}

#elif defined(PLATFORM_WINDOWS) && defined(COMPILER_MSVC)
    #pragma warning(disable : 4206)
#endif
