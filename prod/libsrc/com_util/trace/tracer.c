/**
 *******************************************************************************
 *  @file           tracer.c
 *  @brief          トレース プロバイダー実装ファイル。
 *  @author         Tetsuo Honda
 *  @date           2026/04/03
 *  @version        1.0.0
 *
 *  com_util_tracer_create / com_util_tracer_dispose / com_util_tracer_start / com_util_tracer_stop / com_util_tracer_write など
 *  公開 API の実装と、内部レジストリ管理機能を提供します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */
#include <com_util/clock/clock.h>
#include <com_util/runtime/process.h>
#include <com_util/sync/sync.h>
#include <com_util/trace/tracer.h>
#include <com_util/trace/trace_file.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <inttypes.h>

#include <com_util/trace/tracer_internal.h>
#include <com_util/trace/backends/file/trace_file_internal.h>

#if defined(PLATFORM_LINUX)
    #include <com_util/trace/backends/syslog/syslog_internal.h>
#elif defined(PLATFORM_WINDOWS)
    #include <com_util/trace/backends/etw/etw_internal.h>
#endif /* PLATFORM_ */

/* ===== Windows: TraceLogging プロバイダー定義 ===== */

#if defined(PLATFORM_WINDOWS)

    #include <com_util/base/windows_sdk.h>
    #include <TraceLoggingProvider.h>

TRACELOGGING_DEFINE_PROVIDER(s_trace_provider_ref, COM_UTIL_TRACER_DEFAULT_PROVIDER_NAME,
                             COM_UTIL_TRACER_DEFAULT_PROVIDER_GUID);

static volatile LONG s_trace_ref = 0;
static com_util_etw_provider *s_etw_handle = NULL;
static com_util_local_lock *s_registry_lock;
static com_util_once_flag s_registry_lock_once = {0};

#elif defined(PLATFORM_LINUX)

    #include <syslog.h>
    #include <unistd.h>

static com_util_local_lock *s_registry_lock;
static com_util_once_flag s_registry_lock_once = {0};

#endif /* PLATFORM_ */

/** プロセス名取得失敗時のフォールバック名。 */
#define FALLBACK_NAME "unknown"

/** registry の初期容量。 */
#define TRACE_REGISTRY_INITIAL_CAPACITY 8

enum trace_handle_state
{
    TRACE_HANDLE_ACTIVE = 0,
    TRACE_HANDLE_DISPOSING,
    TRACE_HANDLE_DISPOSED
};

/**
 *  @brief  トレース フック エントリ構造体 (内部定義)。
 */
struct com_util_tracer_hook_entry
{
    com_util_tracer_hook_fn_t fn;
    void *context;
    struct com_util_tracer_hook_entry *next;
};

/**
 *  @brief  トレース プロバイダー ハンドル構造体 (内部定義)。
 */
struct com_util_tracer
{
    int64_t identifier;

#if defined(PLATFORM_LINUX)
    com_util_syslog_sink *syslog_handle;
#elif defined(PLATFORM_WINDOWS)
    char *service_name;
#endif /* PLATFORM_ */

    com_util_trace_file_sink *file_handle;

    com_util_local_rwlock *config_rwlock;

    com_util_trace_level_t os_level;
    com_util_trace_level_t file_level;
    com_util_trace_level_t stderr_level;
    volatile int running;
    volatile int lifecycle_state;

    int config_rwlock_initialized;

    com_util_tracer_hook_entry *hook_head;
};

struct trace_registry
{
    com_util_tracer **items;
    size_t count;
    size_t capacity;
    volatile size_t shutdown_started;
};

static struct trace_registry s_trace_registry = {0};
static com_util_once_flag s_trace_shutdown_once = {0};

static void trace_shutdown_callback(const com_util_shutdown_event *event, void *context);

static void init_registry_lock(void)
{
    (void)com_util_local_lock_create(&s_registry_lock);
}

static void register_trace_shutdown_callback(void)
{
    (void)com_util_shutdown_register(trace_shutdown_callback, NULL);
}

static void trace_shutdown_callback(const com_util_shutdown_event *event, void *context)
{
    (void)context;
    trace_registry_dispose_all_on_shutdown(event);
}

/**
 *  @brief          レジストリの排他ロックを取得する。
 */
static void registry_lock(void)
{
    com_util_call_once(&s_registry_lock_once, init_registry_lock);
    com_util_local_lock_lock(s_registry_lock, COM_UTIL_SYNC_WAIT_FOREVER);
}

/**
 *  @brief          レジストリの排他ロックを解放する。
 */
static void registry_unlock(void)
{
    com_util_local_lock_unlock(s_registry_lock);
}

/**
 *  @brief          レジストリを拡張する (ロック保持中)。
 *  @return         成功時 0、メモリ確保失敗時 -1。
 */
static int registry_expand_locked(void)
{
    com_util_tracer **new_items;
    size_t new_capacity;

    new_capacity = (s_trace_registry.capacity == 0) ? TRACE_REGISTRY_INITIAL_CAPACITY : s_trace_registry.capacity * 2;

    new_items = (com_util_tracer **)realloc(s_trace_registry.items, new_capacity * sizeof(com_util_tracer *));
    if (new_items == NULL)
    {
        return -1;
    }

    s_trace_registry.items = new_items;
    s_trace_registry.capacity = new_capacity;
    return 0;
}

/**
 *  @brief          ハンドルをレジストリに登録する。
 *  @param[in]      handle  登録するトレース プロバイダー ハンドル。
 *  @return         成功時 0、シャットダウン中またはメモリ不足時 -1。
 */
static int registry_register_handle(com_util_tracer *handle)
{
    int rc = 0;

    registry_lock();

    if (s_trace_registry.shutdown_started)
    {
        rc = -1;
    }
    else
    {
        if (s_trace_registry.count == s_trace_registry.capacity)
        {
            rc = registry_expand_locked();
        }
        if (rc == 0)
        {
            s_trace_registry.items[s_trace_registry.count++] = handle;
        }
    }

    registry_unlock();
    return rc;
}

/**
 *  @brief          ハンドルをレジストリから削除する。
 *  @param[in]      handle  削除するトレース プロバイダー ハンドル。
 */
static void registry_unregister_handle(com_util_tracer *handle)
{
    size_t i;

    registry_lock();

    for (i = 0; i < s_trace_registry.count; i++)
    {
        if (s_trace_registry.items[i] == handle)
        {
            s_trace_registry.count--;
            s_trace_registry.items[i] = s_trace_registry.items[s_trace_registry.count];
            s_trace_registry.items[s_trace_registry.count] = NULL;
            break;
        }
    }

    registry_unlock();
}

size_t trace_registry_count(void)
{
    size_t count;

    registry_lock();
    count = s_trace_registry.count;
    registry_unlock();
    return count;
}

size_t trace_registry_capacity(void)
{
    size_t capacity;

    registry_lock();
    capacity = s_trace_registry.capacity;
    registry_unlock();
    return capacity;
}

/**
 *  @brief          ハンドルがアクティブか判定する。
 *  @param[in]      handle  判定対象のトレース プロバイダー ハンドル。
 *  @return         アクティブの場合 1、それ以外 0。
 */
static int handle_is_active(const com_util_tracer *handle)
{
    return handle != NULL && handle->lifecycle_state == TRACE_HANDLE_ACTIVE && !s_trace_registry.shutdown_started;
}

/**
 *  @brief          解放処理を開始する。
 *  @param[in]      handle  解放対象のトレース プロバイダー ハンドル。
 *  @return         成功時 0、ハンドルが NULL またはアクティブでない場合 -1。
 */
static int begin_dispose(com_util_tracer *handle)
{
    if (handle == NULL || handle->lifecycle_state != TRACE_HANDLE_ACTIVE)
    {
        return -1;
    }

    handle->lifecycle_state = TRACE_HANDLE_DISPOSING;
    return 0;
}

#if defined(PLATFORM_LINUX)

/**
 *  @brief          トレース レベルを syslog レベルに変換する。
 *  @param[in]      lv  変換元のトレース レベル。
 *  @return         対応する syslog レベル値。
 */
static int to_syslog_level(const com_util_trace_level_t lv)
{
    switch (lv)
    {
    case COM_UTIL_TRACE_LEVEL_CRITICAL:
        return LOG_CRIT;
    case COM_UTIL_TRACE_LEVEL_ERROR:
        return LOG_ERR;
    case COM_UTIL_TRACE_LEVEL_WARNING:
        return LOG_WARNING;
    case COM_UTIL_TRACE_LEVEL_INFO:
        return LOG_INFO;
    case COM_UTIL_TRACE_LEVEL_VERBOSE:
        return LOG_DEBUG;
    case COM_UTIL_TRACE_LEVEL_DEBUG:
        return LOG_DEBUG;
    case COM_UTIL_TRACE_LEVEL_NONE:
    default:
        return LOG_DEBUG;
    }
}

#elif defined(PLATFORM_WINDOWS)

/**
 *  @brief          トレース レベルを ETW レベルに変換する。
 *  @param[in]      lv  変換元のトレース レベル。
 *  @return         対応する ETW レベル値。
 */
static int to_etw_level(const com_util_trace_level_t lv)
{
    switch (lv)
    {
    case COM_UTIL_TRACE_LEVEL_CRITICAL:
        return 1;
    case COM_UTIL_TRACE_LEVEL_ERROR:
        return 2;
    case COM_UTIL_TRACE_LEVEL_WARNING:
        return 3;
    case COM_UTIL_TRACE_LEVEL_INFO:
        return 4;
    case COM_UTIL_TRACE_LEVEL_VERBOSE:
        return 5;
    case COM_UTIL_TRACE_LEVEL_DEBUG:
        return 5;
    default:
        return 5;
    }
}

#endif /* PLATFORM_ */

/**
 *  @brief          プロセスの実行ファイル パスからベース名を取得する。
 *  @param[in,out]  buf       パス文字列を格納するバッファー。
 *  @param[in]      buf_size  バッファーのバイト数。
 *  @return         ベース名へのポインター。失敗時は FALLBACK_NAME。
 *
 *  com_util_process_get_executable_path() で取得したパスは UTF-8 で
 *  セパレーターが '/' に統一されるため、プラットフォーム非依存で処理できる。
 */
static const char *get_process_basename(char *buf, const size_t buf_size)
{
    const char *sep;

    if (com_util_process_get_executable_path(buf, buf_size) != 0)
    {
        return FALLBACK_NAME;
    }

    sep = strrchr(buf, '/');
    if (sep != NULL)
    {
        return sep + 1;
    }
    return buf;
}

/**
 *  @brief          設定の排他ロック (書き込みロック) を取得する。
 *  @param[in]      handle  ロック対象のトレース プロバイダー ハンドル。
 */
static void config_lock_exclusive(com_util_tracer *handle)
{
    com_util_local_rwlock_lock_exclusive(handle->config_rwlock, COM_UTIL_SYNC_WAIT_FOREVER);
}

/**
 *  @brief          設定の排他ロック (書き込みロック) を解放する。
 *  @param[in]      handle  ロック解放対象のトレース プロバイダー ハンドル。
 */
static void config_unlock_exclusive(com_util_tracer *handle)
{
    com_util_local_rwlock_unlock_exclusive(handle->config_rwlock);
}

#define LOCK_TIMEOUT_MS 100

/**
 *  @brief          タイムアウト付きで設定の共有ロック (読み取りロック) を取得する。
 *  @param[in]      handle  ロック対象のトレース プロバイダー ハンドル。
 *  @return         成功時 0、タイムアウト時 -1。
 */
static int config_lock_shared_timed(com_util_tracer *handle)
{
    if (com_util_local_rwlock_lock_shared(handle->config_rwlock, LOCK_TIMEOUT_MS) == 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/**
 *  @brief          設定の共有ロック (読み取りロック) を解放する。
 *  @param[in]      handle  ロック解放対象のトレース プロバイダー ハンドル。
 */
static void config_unlock_shared(com_util_tracer *handle)
{
    com_util_local_rwlock_unlock_shared(handle->config_rwlock);
}

/**
 *  @brief          識別子を付加した有効名の文字列を構築する。
 *  @param[in]      name        サービス名 (NULL の場合はプロセス ベース名を使用)。
 *  @param[in]      identifier  識別子 (0 の場合はサフィックスなし)。
 *  @return         ヒープ確保された有効名文字列。呼び出し元が free すること。
 *                  失敗時は NULL。
 */
static char *build_effective_name(const char *name, const int64_t identifier)
{
    char path_buf[256];
    const char *base;
    char *result;

    if (name != NULL)
    {
        base = name;
    }
    else
    {
        base = get_process_basename(path_buf, sizeof(path_buf));
    }

    if (identifier == 0)
    {
#if defined(PLATFORM_LINUX)
        return strdup(base);
#elif defined(PLATFORM_WINDOWS)
        return _strdup(base);
#endif /* PLATFORM_ */
    }

    {
        int id_len = snprintf(NULL, 0, "%" PRId64, identifier);
        size_t base_len = strlen(base);
        size_t total = base_len + 1 + (size_t)id_len + 1;

        result = (char *)malloc(total);
        if (result == NULL)
        {
            return NULL;
        }
        snprintf(result, total, "%s-%" PRId64, base, identifier);
        return result;
    }
}

/**
 *  @brief          クリーンアップのためハンドルのトレース出力を停止する。
 *  @param[in]      handle  停止対象のトレース プロバイダー ハンドル。
 *  @return         常に 0。
 */
static int stop_handle_for_cleanup(com_util_tracer *handle)
{
    config_lock_exclusive(handle);
    if (!handle->running)
    {
        config_unlock_exclusive(handle);
        return 0;
    }

    handle->running = 0;
    config_unlock_exclusive(handle);
    return 0;
}

/**
 *  @brief          通常のハンドル解放処理を行う。
 *  @param[in]      handle  解放対象のトレース プロバイダー ハンドル。
 */
static void trace_handle_release_normal(com_util_tracer *handle)
{
    com_util_tracer_hook_entry *hook;
    com_util_tracer_hook_entry *next;

    if (handle->file_handle != NULL)
    {
        com_util_trace_file_sink_dispose(handle->file_handle);
        handle->file_handle = NULL;
    }

    for (hook = handle->hook_head; hook != NULL; hook = next)
    {
        next = hook->next;
        free(hook);
    }
    handle->hook_head = NULL;

#if defined(PLATFORM_LINUX)
    com_util_syslog_sink_dispose(handle->syslog_handle);
    if (handle->config_rwlock_initialized)
    {
        com_util_local_rwlock_destroy(handle->config_rwlock);
    }
#elif defined(PLATFORM_WINDOWS)
    if (InterlockedDecrement(&s_trace_ref) == 0)
    {
        com_util_etw_provider_dispose(s_etw_handle);
        s_etw_handle = NULL;
    }
    free(handle->service_name);
#endif /* PLATFORM_ */

    handle->lifecycle_state = TRACE_HANDLE_DISPOSED;
    free(handle);
}

/**
 *  @brief          アンロード時のハンドル解放処理を行う。
 *  @param[in]      handle  解放対象のトレース プロバイダー ハンドル。
 */
static void trace_handle_release_on_shutdown(com_util_tracer *handle)
{
    if (handle->file_handle != NULL)
    {
        com_util_trace_file_sink_dispose_on_shutdown(handle->file_handle);
        handle->file_handle = NULL;
    }

#if defined(PLATFORM_LINUX)
    com_util_syslog_sink_dispose_on_shutdown(handle->syslog_handle);
#elif defined(PLATFORM_WINDOWS)
    free(handle->service_name);
#endif /* PLATFORM_ */

    handle->lifecycle_state = TRACE_HANDLE_DISPOSED;
    free(handle);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT com_util_tracer *COM_UTIL_API com_util_tracer_create(void)
{
    com_util_tracer *handle;
    char path_buf[256];
    const char *effective_name;

    com_util_call_once(&s_trace_shutdown_once, register_trace_shutdown_callback);

    if (s_trace_registry.shutdown_started)
    {
        return NULL;
    }

    effective_name = get_process_basename(path_buf, sizeof(path_buf));

#if defined(PLATFORM_LINUX)
    {
        com_util_syslog_sink *sp;

        sp = com_util_syslog_sink_create(effective_name, LOG_USER);
        if (sp == NULL)
        {
            return NULL;
        }

        handle = (com_util_tracer *)malloc(sizeof(com_util_tracer));
        if (handle == NULL)
        {
            com_util_syslog_sink_dispose(sp);
            return NULL;
        }

        handle->identifier = 0;
        handle->syslog_handle = sp;
        handle->os_level = COM_UTIL_TRACER_DEFAULT_OS_LEVEL;
        handle->file_level = COM_UTIL_TRACER_DEFAULT_FILE_LEVEL;
        handle->file_handle = NULL;
        handle->stderr_level = COM_UTIL_TRACER_DEFAULT_STDERR_LEVEL;
        handle->running = 0;
        handle->lifecycle_state = TRACE_HANDLE_ACTIVE;
        handle->config_rwlock_initialized = 0;
        handle->hook_head = NULL;

        if (com_util_local_rwlock_create(&handle->config_rwlock) != 0)
        {
            com_util_syslog_sink_dispose(sp);
            free(handle);
            return NULL;
        }
        handle->config_rwlock_initialized = 1;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        char *svc;

        svc = _strdup(effective_name);
        if (svc == NULL)
        {
            return NULL;
        }

        handle = (com_util_tracer *)malloc(sizeof(com_util_tracer));
        if (handle == NULL)
        {
            free(svc);
            return NULL;
        }

        handle->identifier = 0;
        handle->service_name = svc;
        handle->os_level = COM_UTIL_TRACER_DEFAULT_OS_LEVEL;
        handle->file_level = COM_UTIL_TRACER_DEFAULT_FILE_LEVEL;
        handle->file_handle = NULL;
        handle->stderr_level = COM_UTIL_TRACER_DEFAULT_STDERR_LEVEL;
        handle->running = 0;
        handle->lifecycle_state = TRACE_HANDLE_ACTIVE;
        handle->config_rwlock_initialized = 0;
        handle->hook_head = NULL;

        if (com_util_local_rwlock_create(&handle->config_rwlock) != 0)
        {
            free(handle->service_name);
            free(handle);
            return NULL;
        }
        handle->config_rwlock_initialized = 1;

        if (InterlockedIncrement(&s_trace_ref) == 1)
        {
            s_etw_handle = com_util_etw_provider_create(s_trace_provider_ref);
            if (s_etw_handle == NULL)
            {
                InterlockedDecrement(&s_trace_ref);
                free(handle->service_name);
                free(handle);
                return NULL;
            }
        }
    }
#endif /* PLATFORM_ */

    if (registry_register_handle(handle) != 0)
    {
#if defined(PLATFORM_LINUX)
        com_util_syslog_sink_dispose(handle->syslog_handle);
#elif defined(PLATFORM_WINDOWS)
        if (InterlockedDecrement(&s_trace_ref) == 0)
        {
            com_util_etw_provider_dispose(s_etw_handle);
            s_etw_handle = NULL;
        }
        free(handle->service_name);
#endif /* PLATFORM_ */
        if (handle->config_rwlock_initialized)
        {
            com_util_local_rwlock_destroy(handle->config_rwlock);
        }
        free(handle);
        return NULL;
    }

    return handle;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_tracer_start(com_util_tracer *handle)
{
    if (!handle_is_active(handle))
    {
        return -1;
    }

    config_lock_exclusive(handle);
    if (handle->lifecycle_state != TRACE_HANDLE_ACTIVE)
    {
        config_unlock_exclusive(handle);
        return -1;
    }
    if (handle->running)
    {
        config_unlock_exclusive(handle);
        return 0;
    }

    handle->running = 1;
    config_unlock_exclusive(handle);
    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_tracer_stop(com_util_tracer *handle)
{
    if (!handle_is_active(handle))
    {
        return -1;
    }

    return stop_handle_for_cleanup(handle);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT com_util_tracer_state_t COM_UTIL_API com_util_tracer_get_state(com_util_tracer *handle)
{
    com_util_tracer_state_t state;

    if (!handle_is_active(handle))
    {
        return COM_UTIL_TRACER_STATE_DISPOSED;
    }
    if (config_lock_shared_timed(handle) != 0)
    {
        return COM_UTIL_TRACER_STATE_DISPOSED;
    }
    if (handle->lifecycle_state != TRACE_HANDLE_ACTIVE)
    {
        config_unlock_shared(handle);
        return COM_UTIL_TRACER_STATE_DISPOSED;
    }

    state = handle->running ? COM_UTIL_TRACER_STATE_STARTED : COM_UTIL_TRACER_STATE_STOPPED;
    config_unlock_shared(handle);
    return state;
}

#define MAX_BODY (COM_UTIL_TRACER_MESSAGE_MAX_BYTES - 1)

/**
 *  @brief          UTF-8 文字境界を考慮して文字列を切り詰める位置を返す。
 *  @param[in]      s    切り詰め対象の UTF-8 文字列。
 *  @param[in]      pos  切り詰め開始位置 (バイト単位)。
 *  @return         文字境界に合わせた切り詰め位置。
 */
static size_t utf8_safe_truncate(const char *s, const size_t pos)
{
    size_t result = pos;

    while (result > 0 && ((unsigned char)s[result] & 0xC0) == 0x80)
    {
        result--;
    }
    return result;
}

/**
 *  @brief          OS トレース プロバイダー (ETW または syslog) にメッセージを書き込む。
 *  @param[in]      handle      書き込み先のトレース プロバイダー ハンドル。
 *  @param[in]      level       トレース レベル。
 *  @param[in]      timestamp   書き込みに使用する実時刻。NULL の場合は内部で現在時刻を取得。
 *  @param[in]      msg         書き込むメッセージ文字列。
 *  @return         成功時 0、失敗時 -1。
 */
static int write_to_provider(com_util_tracer *handle, const com_util_trace_level_t level,
                             const com_util_realtime_timestamp *timestamp, const char *msg)
{
#if defined(PLATFORM_LINUX)
    return com_util_syslog_sink_write(handle->syslog_handle, to_syslog_level(level), timestamp, msg);
#elif defined(PLATFORM_WINDOWS)
    (void)timestamp;
    return com_util_etw_provider_write(s_etw_handle, to_etw_level(level), handle->service_name, msg);
#endif /* PLATFORM_ */
}

/**
 *  @brief          メッセージを出力すべきか判定する。
 *  @param[in]      msg_level   出力するメッセージのトレース レベル。
 *  @param[in]      threshold   出力閾値となるトレース レベル。
 *  @return         出力すべき場合 1、出力不要の場合 0。
 */
static int should_output(const com_util_trace_level_t msg_level, const com_util_trace_level_t threshold)
{
    if (threshold == COM_UTIL_TRACE_LEVEL_NONE)
    {
        return 0;
    }
    return (int)msg_level <= (int)threshold;
}

#define STDERR_TS_BUF_SIZE (COM_UTIL_CLOCK_ISO8601_LOCAL_MSEC_LEN + 1)

static int timestamp_is_valid(const com_util_realtime_timestamp *timestamp)
{
    return timestamp != NULL && timestamp->tv_nsec >= 0 && timestamp->tv_nsec < 1000000000;
}

static int resolve_timestamp(const com_util_realtime_timestamp *timestamp, com_util_realtime_timestamp *resolved,
                             int *fallback_used)
{
    if (resolved == NULL)
    {
        return -1;
    }
    if (fallback_used != NULL)
    {
        *fallback_used = 0;
    }

    if (timestamp != NULL)
    {
        if (timestamp_is_valid(timestamp))
        {
            *resolved = *timestamp;
            return 0;
        }
        if (fallback_used != NULL)
        {
            *fallback_used = 1;
        }
    }

    com_util_get_realtime(&resolved->tv_sec, &resolved->tv_nsec);
    if (timestamp_is_valid(resolved))
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

static int format_local_timestamp(char *buf, const size_t buf_size, const com_util_realtime_timestamp *timestamp)
{
    if (!timestamp_is_valid(timestamp))
    {
        return -1;
    }
    return com_util_format_realtime_iso8601_local(buf, buf_size, timestamp->tv_sec, timestamp->tv_nsec);
}

/**
 *  @brief          タイムスタンプとトレース レベルを付加して stderr にエントリを書き込む。
 *  @param[in]      level  トレース レベル。
 *  @param[in]      timestamp_text  事前整形済みタイムスタンプ文字列。
 *  @param[in]      msg    書き込むメッセージ文字列。
 */
static void write_stderr_entry(const com_util_trace_level_t level, const char *timestamp_text, const char *msg)
{
    static const char lc_table[] = {'C', 'E', 'W', 'I', 'V', 'D'};
    char lc;

    if ((int)level >= 0 && (int)level < (int)COM_UTIL_TRACE_LEVEL_NONE)
    {
        lc = lc_table[(int)level];
    }
    else
    {
        lc = 'D';
    }
    fprintf(stderr, "%s %c %s\n", timestamp_text, lc, msg);
}

/**
 *  @brief          OS プロバイダ・ファイル・stderr の各出力先にメッセージを書き込む。
 *  @param[in]      handle  書き込み先のトレース プロバイダー ハンドル。
 *  @param[in]      level   トレース レベル。
 *  @param[in]      timestamp  書き込みに使用する実時刻。NULL の場合は内部で現在時刻を取得。
 *  @param[in]      msg        書き込むメッセージ文字列。
 *  @return         全出力先で成功時 0、いずれかで失敗時 -1。
 */
static int write_dual(com_util_tracer *handle, const com_util_trace_level_t level,
                      const com_util_realtime_timestamp *timestamp, const char *msg)
{
    int os_result = 0;
    int file_result = 0;
    int timestamp_fallback_used = 0;
    int needs_text_timestamp;
    com_util_realtime_timestamp resolved;
    const com_util_realtime_timestamp *effective_timestamp = NULL;
    char ts[STDERR_TS_BUF_SIZE];

    needs_text_timestamp = (handle->file_handle != NULL && should_output(level, handle->file_level)) ||
                           should_output(level, handle->stderr_level) ||
#if defined(PLATFORM_LINUX)
                           should_output(level, handle->os_level) ||
#endif
                           handle->hook_head != NULL || timestamp != NULL;

    if (needs_text_timestamp)
    {
        if (resolve_timestamp(timestamp, &resolved, &timestamp_fallback_used) != 0)
        {
            return -1;
        }
        effective_timestamp = &resolved;

        if (format_local_timestamp(ts, sizeof(ts), effective_timestamp) != 0)
        {
            return -1;
        }
    }

    if (should_output(level, handle->os_level))
    {
        os_result = write_to_provider(handle, level, effective_timestamp, msg);
    }

    if (handle->file_handle != NULL && should_output(level, handle->file_level))
    {
        file_result = com_util_trace_file_sink_write(handle->file_handle, (int)level, effective_timestamp, msg);
    }

    if (should_output(level, handle->stderr_level))
    {
        write_stderr_entry(level, ts, msg);
    }

    if (handle->hook_head != NULL)
    {
        com_util_tracer_hook_entry *head = handle->hook_head;
        if (head->fn != NULL)
        {
            head->fn(head->next, handle, level, effective_timestamp, msg, head->context);
        }
    }

    if (timestamp_fallback_used || os_result != 0 || file_result != 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API _com_util_tracer_write(com_util_tracer *handle, const com_util_trace_level_t level,
                                                        const com_util_realtime_timestamp *timestamp,
                                                        const char *message)
{
    const char *msg;
    char buf[COM_UTIL_TRACER_MESSAGE_MAX_BYTES];
    size_t len;
    int ret;

    if (handle == NULL || message == NULL)
    {
        return 0;
    }
    if (!handle_is_active(handle))
    {
        return -1;
    }
    if (config_lock_shared_timed(handle) != 0)
    {
        return -1;
    }
    if (handle->lifecycle_state != TRACE_HANDLE_ACTIVE || !handle->running)
    {
        config_unlock_shared(handle);
        return -1;
    }

    msg = message;
    len = strlen(message);
    if (len > MAX_BODY)
    {
        size_t safe_len = utf8_safe_truncate(message, MAX_BODY);
        memcpy(buf, message, safe_len);
        buf[safe_len] = '\0';
        msg = buf;
    }

    ret = write_dual(handle, level, timestamp, msg);
    config_unlock_shared(handle);
    return ret;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API _com_util_tracer_writef(com_util_tracer *handle, const com_util_trace_level_t level,
                                                         const com_util_realtime_timestamp *timestamp,
                                                         const char *format, ...)
{
    va_list args;
    char buf[COM_UTIL_TRACER_MESSAGE_MAX_BYTES];
    int ret;

    if (handle == NULL || format == NULL)
    {
        return 0;
    }
    if (!handle_is_active(handle))
    {
        return -1;
    }
    if (config_lock_shared_timed(handle) != 0)
    {
        return -1;
    }
    if (handle->lifecycle_state != TRACE_HANDLE_ACTIVE || !handle->running)
    {
        config_unlock_shared(handle);
        return -1;
    }

    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    ret = write_dual(handle, level, timestamp, buf);
    config_unlock_shared(handle);
    return ret;
}

static const char hex_chars[] = "0123456789ABCDEF";
#define ELLIPSIS_LEN 3

/**
 *  @brief          16 進ダンプ メッセージを構築して出力先に書き込む内部実装。
 *  @param[in]      handle      書き込み先のトレース プロバイダー ハンドル。
 *  @param[in]      level       トレース レベル。
 *  @param[in]      timestamp   書き込みに使用する実時刻。NULL の場合は内部で現在時刻を取得。
 *  @param[in]      data        ダンプ対象のバイト列。
 *  @param[in]      size        バイト列のサイズ。
 *  @param[in]      label       メッセージに付加するラベル (NULL 可)。
 *  @return         成功時 0、失敗時 -1。
 */
static int hex_write_impl(com_util_tracer *handle, const com_util_trace_level_t level,
                          const com_util_realtime_timestamp *timestamp, const void *data, const size_t size,
                          const char *label)
{
    char buf[COM_UTIL_TRACER_MESSAGE_MAX_BYTES];
    const unsigned char *bytes = (const unsigned char *)data;
    size_t pos = 0;
    size_t remaining;
    size_t max_data_bytes;
    size_t effective_size;
    int truncated = 0;
    size_t i;

    effective_size = size;

    if (handle == NULL || data == NULL || size == 0)
    {
        return 0;
    }

    if (label != NULL && label[0] != '\0')
    {
        size_t lbl_len = strlen(label);
        if (lbl_len + 2 >= MAX_BODY)
        {
            size_t copy_len;
            if (lbl_len < MAX_BODY)
            {
                copy_len = lbl_len;
            }
            else
            {
                copy_len = MAX_BODY;
            }
            memcpy(buf, label, copy_len);
            buf[copy_len] = '\0';
            return write_dual(handle, level, timestamp, buf);
        }
        memcpy(buf, label, lbl_len);
        buf[lbl_len] = ':';
        buf[lbl_len + 1] = ' ';
        pos = lbl_len + 2;
    }

    remaining = MAX_BODY - pos;
    max_data_bytes = (remaining + 1) / 3;

    if (effective_size > max_data_bytes)
    {
        truncated = 1;
        if (remaining < ELLIPSIS_LEN)
        {
            buf[pos] = '\0';
            return write_dual(handle, level, timestamp, buf);
        }
        max_data_bytes = (remaining - ELLIPSIS_LEN) / 3;
        if (max_data_bytes == 0)
        {
            if (pos > (MAX_BODY - ELLIPSIS_LEN))
            {
                buf[pos] = '\0';
                return write_dual(handle, level, timestamp, buf);
            }
            memcpy(buf + pos, "...", ELLIPSIS_LEN);
            pos += ELLIPSIS_LEN;
            buf[pos] = '\0';
            return write_dual(handle, level, timestamp, buf);
        }
        effective_size = max_data_bytes;
    }

    for (i = 0; i < effective_size; i++)
    {
        if (i > 0)
        {
            buf[pos++] = ' ';
        }
        buf[pos++] = hex_chars[(bytes[i] >> 4) & 0x0F];
        buf[pos++] = hex_chars[bytes[i] & 0x0F];
    }

    if (truncated)
    {
        buf[pos++] = ' ';
        buf[pos++] = '.';
        buf[pos++] = '.';
        buf[pos++] = '.';
    }
    buf[pos] = '\0';

    return write_dual(handle, level, timestamp, buf);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API _com_util_tracer_write_hex(com_util_tracer *handle, const com_util_trace_level_t level,
                                                            const com_util_realtime_timestamp *timestamp,
                                                            const void *data, const size_t size, const char *message)
{
    int ret;

    if (handle == NULL || data == NULL || size == 0)
    {
        return 0;
    }
    if (!handle_is_active(handle))
    {
        return -1;
    }
    if (config_lock_shared_timed(handle) != 0)
    {
        return -1;
    }
    if (handle->lifecycle_state != TRACE_HANDLE_ACTIVE || !handle->running)
    {
        config_unlock_shared(handle);
        return -1;
    }

    ret = hex_write_impl(handle, level, timestamp, data, size, message);
    config_unlock_shared(handle);
    return ret;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API _com_util_tracer_write_hexf(com_util_tracer *handle,
                                                             const com_util_trace_level_t level,
                                                             const com_util_realtime_timestamp *timestamp,
                                                             const void *data, const size_t size, const char *format,
                                                             ...)
{
    char label[COM_UTIL_TRACER_MESSAGE_MAX_BYTES];
    int ret;

    if (handle == NULL || data == NULL || size == 0)
    {
        return 0;
    }
    if (!handle_is_active(handle))
    {
        return -1;
    }
    if (config_lock_shared_timed(handle) != 0)
    {
        return -1;
    }
    if (handle->lifecycle_state != TRACE_HANDLE_ACTIVE || !handle->running)
    {
        config_unlock_shared(handle);
        return -1;
    }

    if (format != NULL)
    {
        va_list args;
        va_start(args, format);
        vsnprintf(label, sizeof(label), format, args);
        va_end(args);
        ret = hex_write_impl(handle, level, timestamp, data, size, label);
    }
    else
    {
        ret = hex_write_impl(handle, level, timestamp, data, size, NULL);
    }

    config_unlock_shared(handle);
    return ret;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_tracer_set_name(com_util_tracer *handle, const char *name,
                                                          const int64_t identifier)
{
    char *effective;

    if (!handle_is_active(handle))
    {
        return -1;
    }
    if (identifier < 0)
    {
        return -1;
    }

    config_lock_exclusive(handle);
    if (handle->lifecycle_state != TRACE_HANDLE_ACTIVE || handle->running)
    {
        config_unlock_exclusive(handle);
        return -1;
    }

    effective = build_effective_name(name, identifier);
    if (effective == NULL)
    {
        config_unlock_exclusive(handle);
        return -1;
    }

#if defined(PLATFORM_LINUX)
    {
        int rc = com_util_syslog_sink_rename(handle->syslog_handle, effective);
        free(effective);
        if (rc != 0)
        {
            config_unlock_exclusive(handle);
            return -1;
        }
        handle->identifier = identifier;
        config_unlock_exclusive(handle);
        return 0;
    }
#elif defined(PLATFORM_WINDOWS)
    free(handle->service_name);
    handle->service_name = effective;
    handle->identifier = identifier;
    config_unlock_exclusive(handle);
    return 0;
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT com_util_trace_level_t COM_UTIL_API com_util_tracer_get_os_level(com_util_tracer *handle)
{
    com_util_trace_level_t lv;

    if (!handle_is_active(handle))
    {
        return COM_UTIL_TRACE_LEVEL_NONE;
    }
    if (config_lock_shared_timed(handle) != 0)
    {
        return COM_UTIL_TRACE_LEVEL_NONE;
    }
    if (handle->lifecycle_state != TRACE_HANDLE_ACTIVE)
    {
        config_unlock_shared(handle);
        return COM_UTIL_TRACE_LEVEL_NONE;
    }
    lv = handle->os_level;
    config_unlock_shared(handle);
    return lv;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_tracer_set_os_level(com_util_tracer *handle,
                                                              const com_util_trace_level_t level)
{
    if (!handle_is_active(handle))
    {
        return -1;
    }

    config_lock_exclusive(handle);
    if (handle->lifecycle_state != TRACE_HANDLE_ACTIVE || handle->running)
    {
        config_unlock_exclusive(handle);
        return -1;
    }

    handle->os_level = level;
    config_unlock_exclusive(handle);
    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT com_util_trace_level_t COM_UTIL_API com_util_tracer_get_file_level(com_util_tracer *handle)
{
    com_util_trace_level_t lv;

    if (!handle_is_active(handle))
    {
        return COM_UTIL_TRACE_LEVEL_NONE;
    }
    if (config_lock_shared_timed(handle) != 0)
    {
        return COM_UTIL_TRACE_LEVEL_NONE;
    }
    if (handle->lifecycle_state != TRACE_HANDLE_ACTIVE)
    {
        config_unlock_shared(handle);
        return COM_UTIL_TRACE_LEVEL_NONE;
    }
    lv = handle->file_level;
    config_unlock_shared(handle);
    return lv;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_tracer_set_file_level(com_util_tracer *handle, const char *path,
                                                                const com_util_trace_level_t level,
                                                                const size_t max_bytes, const int generations,
                                                                const int flags)
{
    int result = 0;

    if (!handle_is_active(handle))
    {
        return -1;
    }

    config_lock_exclusive(handle);
    if (handle->lifecycle_state != TRACE_HANDLE_ACTIVE || handle->running)
    {
        config_unlock_exclusive(handle);
        return -1;
    }

    if (handle->file_handle != NULL)
    {
        com_util_trace_file_sink_dispose(handle->file_handle);
        handle->file_handle = NULL;
    }

    handle->file_level = level;
    if (path != NULL)
    {
        handle->file_handle = com_util_trace_file_sink_create(path, max_bytes, generations, flags);
        if (handle->file_handle == NULL)
        {
            result = -1;
        }
    }

    config_unlock_exclusive(handle);
    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT com_util_trace_level_t COM_UTIL_API com_util_tracer_get_stderr_level(com_util_tracer *handle)
{
    com_util_trace_level_t lv;

    if (!handle_is_active(handle))
    {
        return COM_UTIL_TRACE_LEVEL_NONE;
    }
    if (config_lock_shared_timed(handle) != 0)
    {
        return COM_UTIL_TRACE_LEVEL_NONE;
    }
    if (handle->lifecycle_state != TRACE_HANDLE_ACTIVE)
    {
        config_unlock_shared(handle);
        return COM_UTIL_TRACE_LEVEL_NONE;
    }
    lv = handle->stderr_level;
    config_unlock_shared(handle);
    return lv;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_tracer_set_stderr_level(com_util_tracer *handle,
                                                                  const com_util_trace_level_t level)
{
    if (!handle_is_active(handle))
    {
        return -1;
    }

    config_lock_exclusive(handle);
    if (handle->lifecycle_state != TRACE_HANDLE_ACTIVE || handle->running)
    {
        config_unlock_exclusive(handle);
        return -1;
    }

    handle->stderr_level = level;
    config_unlock_exclusive(handle);
    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT void COM_UTIL_API com_util_tracer_dispose(com_util_tracer *handle)
{
    if (handle == NULL)
    {
        return;
    }
    if (begin_dispose(handle) != 0)
    {
        return;
    }

    registry_unregister_handle(handle);
    stop_handle_for_cleanup(handle);
    trace_handle_release_normal(handle);
}

void trace_registry_dispose_all_on_shutdown(const com_util_shutdown_event *event)
{
    com_util_tracer **items;
    size_t count;
    size_t i;

    if (event == NULL || s_trace_registry.shutdown_started)
    {
        return;
    }

    s_trace_registry.shutdown_started = 1;
    items = s_trace_registry.items;
    count = s_trace_registry.count;

    s_trace_registry.items = NULL;
    s_trace_registry.count = 0;
    s_trace_registry.capacity = 0;

    for (i = 0; i < count; i++)
    {
        com_util_tracer *handle = items[i];

        if (handle == NULL || handle->lifecycle_state == TRACE_HANDLE_DISPOSED)
        {
            continue;
        }

        handle->lifecycle_state = TRACE_HANDLE_DISPOSING;
        handle->running = 0;
        trace_handle_release_on_shutdown(handle);
    }

#if defined(PLATFORM_WINDOWS)
    if (s_etw_handle != NULL)
    {
        com_util_etw_provider_dispose_on_shutdown(s_etw_handle, event);
        s_etw_handle = NULL;
    }
    s_trace_ref = 0;
#endif /* PLATFORM_WINDOWS */

    free(items);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT com_util_tracer_hook_entry *COM_UTIL_API com_util_tracer_set_hook(com_util_tracer *handle,
                                                                                  com_util_tracer_hook_fn_t fn,
                                                                                  void *context)
{
    com_util_tracer_hook_entry *entry;

    if (!handle_is_active(handle) || fn == NULL)
    {
        return NULL;
    }

    entry = (com_util_tracer_hook_entry *)malloc(sizeof(com_util_tracer_hook_entry));
    if (entry == NULL)
    {
        return NULL;
    }

    config_lock_exclusive(handle);
    if (handle->lifecycle_state != TRACE_HANDLE_ACTIVE || handle->running)
    {
        config_unlock_exclusive(handle);
        free(entry);
        return NULL;
    }

    entry->fn = fn;
    entry->context = context;
    entry->next = handle->hook_head;
    handle->hook_head = entry;

    config_unlock_exclusive(handle);
    return entry;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT void COM_UTIL_API com_util_tracer_remove_hook(com_util_tracer *handle,
                                                              com_util_tracer_hook_entry *hook_entry)
{
    com_util_tracer_hook_entry **pp;

    if (!handle_is_active(handle) || hook_entry == NULL)
    {
        return;
    }

    config_lock_exclusive(handle);
    if (handle->lifecycle_state != TRACE_HANDLE_ACTIVE || handle->running)
    {
        config_unlock_exclusive(handle);
        return;
    }

    for (pp = &handle->hook_head; *pp != NULL; pp = &(*pp)->next)
    {
        if (*pp == hook_entry)
        {
            *pp = hook_entry->next;
            free(hook_entry);
            break;
        }
    }

    config_unlock_exclusive(handle);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT void COM_UTIL_API com_util_tracer_call_next_hook(com_util_tracer_hook_entry *prev,
                                                                 com_util_tracer *handle,
                                                                 const com_util_trace_level_t level,
                                                                 const com_util_realtime_timestamp *timestamp,
                                                                 const char *message)
{
    if (prev == NULL || prev->fn == NULL)
    {
        return;
    }
    prev->fn(prev->next, handle, level, timestamp, message, prev->context);
}
