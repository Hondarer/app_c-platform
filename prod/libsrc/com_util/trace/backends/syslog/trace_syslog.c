/**
 *******************************************************************************
 *  @file           trace_syslog.c
 *  @brief          syslog プロバイダ実装ファイル。
 *  @author         Tetsuo Honda
 *  @date           2026/04/03
 *  @version        1.0.0
 *
 *  Linux syslog ベースのトレースプロバイダを提供します。\n
 *  /dev/log への UNIX ドメイン SOCK_DGRAM 送信で実装しています。
 *  send(MSG_DONTWAIT) を使用するため、ソケットが詰まっていても
 *  アプリケーションをブロックしません。送信失敗時はメッセージを
 *  drop し、低頻度バックオフでのみ再接続を試みます。\n
 *  環境変数 `SYSLOG_TEST_FD` が設定されている場合は /dev/log の代わりに
 *  その FD に RFC 3164 形式のメッセージを書き込みます (テスト用途)。
 *
 *  @par            スレッドセーフティ
 *  本モジュールはスレッドセーフです。\n
 *  fd・next_connect・backoff_sec の読み書きはすべて reconnect_lock で
 *  保護しています。sendto() は MSG_DONTWAIT で即時返るため、
 *  ロック保持中に実行しても問題ありません。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <com_util/clock/clock.h>
#include <com_util/sync/sync.h>
#include <com_util/trace/syslog.h>
#include <com_util/trace/backends/syslog/syslog_internal.h>
#include <com_util/test/syslog_test.h>

/** /dev/log への UNIX ドメインソケットパス。 */
#define DEVLOG_PATH      "/dev/log"

/** 初回バックオフ間隔 (秒)。 */
#define BACKOFF_INIT_SEC  5

/** バックオフ最大間隔 (秒)。 */
#define BACKOFF_MAX_SEC  60

/** メッセージバッファサイズ (RFC 3164 推奨最大長)。 */
#define SYSLOG_BUF_SIZE  2048

/** SYSLOG_TEST_FD 向けバッファサイズ。timestamp と改行を加味する。 */
#define SYSLOG_DEBUG_BUF_SIZE (SYSLOG_BUF_SIZE + COM_UTIL_CLOCK_ISO8601_LOCAL_MSEC_LEN + 4)

/**
 *  @brief  syslog プロバイダハンドル構造体 (内部定義)。
 */
struct com_util_syslog_sink
{
    /** openlog に相当する識別子文字列 (複製を保持)。 */
    char *ident;

    /**
     *  fd・next_connect・backoff_sec を保護する mutex。
     *  sendto() は MSG_DONTWAIT で即時返るため、ロック保持中に実行する。
     */
    com_util_local_lock_t *reconnect_lock;

    /** 次回接続試行を許可する最早時刻 (time_t)。reconnect_lock で保護。 */
    time_t next_connect;

    /** syslog facility 値 (例: LOG_USER = 8)。 */
    int facility;

    /** mutex が初期化済みであることを示すフラグ。 */
    int lock_initialized;

    /** UNIX ドメインソケット fd。未接続時は -1。reconnect_lock で保護。 */
    int fd;

    /** 現在のバックオフ間隔 (秒)。reconnect_lock で保護。 */
    int backoff_sec;
};

/**
 *******************************************************************************
 *  @brief  タイムスタンプが有効範囲か判定する。
 *******************************************************************************
 */
static int timestamp_is_valid(const com_util_realtime_timestamp_t *timestamp)
{
    return timestamp != NULL && timestamp->tv_nsec >= 0 && timestamp->tv_nsec < 1000000000;
}

/**
 *******************************************************************************
 *  @brief  syslog 出力に使用するタイムスタンプを解決する。
 *  @param[in]      timestamp      呼び出し側が渡した明示タイムスタンプ。NULL 可。
 *  @param[out]     resolved       解決後のタイムスタンプ格納先。
 *  @param[out]     fallback_used  不正タイムスタンプから現在時刻へ代替した場合 1。
 *  @return         成功時 0、解決失敗時 -1。
 *******************************************************************************
 */
static int resolve_timestamp(const com_util_realtime_timestamp_t *timestamp,
                             com_util_realtime_timestamp_t *resolved,
                             int *fallback_used)
{
    if (timestamp == NULL)
    {
        if (fallback_used != NULL)
        {
            *fallback_used = 0;
        }
        return 0;
    }
    if (resolved == NULL)
    {
        return -1;
    }
    if (fallback_used != NULL)
    {
        *fallback_used = 0;
    }

    if (timestamp_is_valid(timestamp))
    {
        *resolved = *timestamp;
        return 0;
    }

    if (fallback_used != NULL)
    {
        *fallback_used = 1;
    }

    com_util_get_realtime(&resolved->tv_sec, &resolved->tv_nsec);
    return timestamp_is_valid(resolved) ? 0 : -1;
}

/**
 *******************************************************************************
 *  @brief  バックオフ値を次段階に進める。ロック保持中に呼ぶこと。
 *******************************************************************************
 */
static void advance_backoff(com_util_syslog_sink_t *h)
{
    int next = h->backoff_sec * 2;

    h->backoff_sec = (next > BACKOFF_MAX_SEC) ? BACKOFF_MAX_SEC : next;
}

/**
 *******************************************************************************
 *  @brief  fd を閉じてバックオフを進める。ロック保持中に呼ぶこと。
 *******************************************************************************
 */
static void close_and_backoff_locked(com_util_syslog_sink_t *h)
{
    if (h->fd >= 0)
    {
        close(h->fd);
        h->fd = -1;
    }
    h->next_connect = time(NULL) + h->backoff_sec;
    advance_backoff(h);
}

/**
 *******************************************************************************
 *  @brief  バックオフ期間を経過していればソケットを開く試みを行う。
 *          ロック保持中に呼ぶこと。
 *******************************************************************************
 */
static void try_open_socket_locked(com_util_syslog_sink_t *h)
{
    time_t now = time(NULL);
    int fd;

    if (now < h->next_connect)
    {
        return; /* バックオフ期間中 */
    }

    fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (fd < 0)
    {
        h->next_connect = now + h->backoff_sec;
        advance_backoff(h);
        return;
    }

    h->fd = fd;
    /* バックオフは送信成功時にリセットする */
}

/* doxygen コメントは、ヘッダに記載 */
COM_UTIL_EXPORT com_util_syslog_sink_t *COM_UTIL_API
    com_util_syslog_sink_create(const char *ident, int facility)
{
    com_util_syslog_sink_t *handle;
    size_t len;

    if (ident == NULL)
    {
        return NULL;
    }

    handle = (com_util_syslog_sink_t *)malloc(sizeof(com_util_syslog_sink_t));
    if (handle == NULL)
    {
        return NULL;
    }

    len = strlen(ident) + 1;
    handle->ident = (char *)malloc(len);
    if (handle->ident == NULL)
    {
        free(handle);
        return NULL;
    }
    memcpy(handle->ident, ident, len);

    handle->facility         = facility;
    handle->fd               = -1;
    handle->next_connect     = 0;
    handle->backoff_sec      = BACKOFF_INIT_SEC;
    handle->lock_initialized = 0;

    if (com_util_local_lock_create(&handle->reconnect_lock) != 0)
    {
        free(handle->ident);
        free(handle);
        return NULL;
    }
    handle->lock_initialized = 1;

    /* 初回接続を試みる (失敗しても構わない) */
    com_util_local_lock_lock(handle->reconnect_lock, COM_UTIL_SYNC_WAIT_FOREVER);
    try_open_socket_locked(handle);
    com_util_local_lock_unlock(handle->reconnect_lock);

    return handle;
}

/* doxygen コメントは、ヘッダに記載 */
COM_UTIL_EXPORT int COM_UTIL_API
    com_util_syslog_sink_write(com_util_syslog_sink_t *handle, int level,
                               const com_util_realtime_timestamp_t *timestamp,
                               const char *message)
{
    char buf[SYSLOG_BUF_SIZE];
    char debug_buf[SYSLOG_DEBUG_BUF_SIZE];
    char timestamp_text[COM_UTIL_CLOCK_ISO8601_LOCAL_MSEC_LEN + 1];
    com_util_realtime_timestamp_t resolved;
    const com_util_realtime_timestamp_t *effective_timestamp = NULL;
    struct sockaddr_un sa;
    const char *test_fd_str;
    int fallback_used = 0;
    int prio;
    int n;
    int debug_len;
    ssize_t sent;

    if (handle == NULL || message == NULL)
    {
        return 0;
    }
    if (resolve_timestamp(timestamp, &resolved, &fallback_used) != 0)
    {
        return -1;
    }
    if (timestamp != NULL)
    {
        effective_timestamp = &resolved;
    }

    /* syslog priority = (facility & ~7) | (severity & 7) */
    prio = (handle->facility & ~7) | (level & 7);

    /* RFC 3164 形式: <PRI>TAG[PID]: MSG */
    n = snprintf(buf, sizeof(buf), "<%d>%s[%d]: %s",
                 prio, handle->ident, (int)getpid(), message);
    if (n < 0)
    {
        return 0;
    }
    if ((size_t)n >= sizeof(buf))
    {
        n = (int)(sizeof(buf) - 1);
    }

    /* SYSLOG_TEST_FD が設定されていればテスト用 FD に送信し、/dev/log へは送信しない */
    test_fd_str = getenv("SYSLOG_TEST_FD");
    if (test_fd_str != NULL)
    {
        if (effective_timestamp != NULL &&
            com_util_format_realtime_iso8601_local(timestamp_text, sizeof(timestamp_text),
                                                   effective_timestamp->tv_sec,
                                                   effective_timestamp->tv_nsec) == 0)
        {
            debug_len = snprintf(debug_buf, sizeof(debug_buf), "%s %.*s\n", timestamp_text, n, buf);
            if (debug_len < 0)
            {
                return -1;
            }
            if ((size_t)debug_len >= sizeof(debug_buf))
            {
                debug_buf[sizeof(debug_buf) - 2] = '\n';
                debug_buf[sizeof(debug_buf) - 1] = '\0';
                debug_len = (int)(sizeof(debug_buf) - 1);
            }
            (void)syslog_test_fd_write__(debug_buf, (size_t)debug_len);
        }
        else
        {
            buf[n] = '\n';
            (void)syslog_test_fd_write__(buf, (size_t)(n + 1));
        }
        return fallback_used ? -1 : 0;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, DEVLOG_PATH, sizeof(sa.sun_path) - 1);

    com_util_local_lock_lock(handle->reconnect_lock, COM_UTIL_SYNC_WAIT_FOREVER);

    /* ソケットが無ければ低頻度で再接続を試みる */
    if (handle->fd < 0)
    {
        try_open_socket_locked(handle);
        if (handle->fd < 0)
        {
            com_util_local_lock_unlock(handle->reconnect_lock);
            return fallback_used ? -1 : 0; /* drop */
        }
    }

    /* sendto は MSG_DONTWAIT で即時返るため、ロック保持中に実行する */
    sent = sendto(handle->fd, buf, (size_t)n, MSG_DONTWAIT,
                  (struct sockaddr *)&sa,
                  (socklen_t)sizeof(sa));

    if (sent < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            /* 送信バッファ満杯: drop のみ、再接続不要 */
            com_util_local_lock_unlock(handle->reconnect_lock);
            return fallback_used ? -1 : 0;
        }
        /* その他エラー (ENOENT, ECONNREFUSED 等): ソケットを閉じてバックオフ */
        close_and_backoff_locked(handle);
        com_util_local_lock_unlock(handle->reconnect_lock);
        return fallback_used ? -1 : 0; /* drop */
    }

    /* 送信成功: バックオフをリセット */
    handle->backoff_sec = BACKOFF_INIT_SEC;

    com_util_local_lock_unlock(handle->reconnect_lock);
    return fallback_used ? -1 : 0;
}

/* doxygen コメントは、ヘッダに記載 */
COM_UTIL_EXPORT void COM_UTIL_API
    com_util_syslog_sink_dispose(com_util_syslog_sink_t *handle)
{
    if (handle == NULL)
    {
        return;
    }

    if (handle->fd >= 0)
    {
        close(handle->fd);
    }
    if (handle->lock_initialized)
    {
        com_util_local_lock_destroy(handle->reconnect_lock);
    }
    free(handle->ident);
    free(handle);
}

/* doxygen コメントは、ヘッダに記載 */
COM_UTIL_EXPORT int COM_UTIL_API
    com_util_syslog_sink_rename(com_util_syslog_sink_t *handle, const char *new_ident)
{
    char *dup;
    size_t len;

    if (handle == NULL || new_ident == NULL)
    {
        return -1;
    }

    len = strlen(new_ident) + 1;
    dup = (char *)malloc(len);
    if (dup == NULL)
    {
        return -1;
    }
    memcpy(dup, new_ident, len);

    com_util_local_lock_lock(handle->reconnect_lock, COM_UTIL_SYNC_WAIT_FOREVER);
    free(handle->ident);
    handle->ident = dup;
    com_util_local_lock_unlock(handle->reconnect_lock);

    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
void com_util_syslog_sink_dispose_on_shutdown(com_util_syslog_sink_t *handle)
{
    if (handle == NULL)
    {
        return;
    }

    if (handle->fd >= 0)
    {
        close(handle->fd);
    }
    if (handle->lock_initialized)
    {
        com_util_local_lock_destroy(handle->reconnect_lock);
    }
    free(handle->ident);
    free(handle);
}

#elif defined(PLATFORM_WINDOWS) && defined(COMPILER_MSVC)
    #pragma warning(disable : 4206)
#endif
