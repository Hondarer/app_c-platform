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

#include <com_util/sync/sync.h>
#include <com_util/trace/syslog.h>
#include "syslog_internal.h"
#include <com_util/test/syslog_test.h>

/** /dev/log への UNIX ドメインソケットパス。 */
#define DEVLOG_PATH      "/dev/log"

/** 初回バックオフ間隔 (秒)。 */
#define BACKOFF_INIT_SEC  5

/** バックオフ最大間隔 (秒)。 */
#define BACKOFF_MAX_SEC  60

/** メッセージバッファサイズ (RFC 3164 推奨最大長)。 */
#define SYSLOG_BUF_SIZE  2048

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
    com_util_mutex_t reconnect_lock;

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

    if (com_util_mutex_init(&handle->reconnect_lock) != 0)
    {
        free(handle->ident);
        free(handle);
        return NULL;
    }
    handle->lock_initialized = 1;

    /* 初回接続を試みる (失敗しても構わない) */
    com_util_mutex_lock(&handle->reconnect_lock);
    try_open_socket_locked(handle);
    com_util_mutex_unlock(&handle->reconnect_lock);

    return handle;
}

/* doxygen コメントは、ヘッダに記載 */
COM_UTIL_EXPORT int COM_UTIL_API
    com_util_syslog_sink_write(com_util_syslog_sink_t *handle, int level, const char *message)
{
    char buf[SYSLOG_BUF_SIZE];
    struct sockaddr_un sa;
    int prio;
    int n;
    ssize_t sent;

    if (handle == NULL || message == NULL)
    {
        return 0;
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
    buf[n] = '\n';
    if (syslog_test_fd_write__(buf, (size_t)(n + 1)))
    {
        return 0;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, DEVLOG_PATH, sizeof(sa.sun_path) - 1);

    com_util_mutex_lock(&handle->reconnect_lock);

    /* ソケットが無ければ低頻度で再接続を試みる */
    if (handle->fd < 0)
    {
        try_open_socket_locked(handle);
        if (handle->fd < 0)
        {
            com_util_mutex_unlock(&handle->reconnect_lock);
            return 0; /* drop */
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
            com_util_mutex_unlock(&handle->reconnect_lock);
            return 0;
        }
        /* その他エラー (ENOENT, ECONNREFUSED 等): ソケットを閉じてバックオフ */
        close_and_backoff_locked(handle);
        com_util_mutex_unlock(&handle->reconnect_lock);
        return 0; /* drop */
    }

    /* 送信成功: バックオフをリセット */
    handle->backoff_sec = BACKOFF_INIT_SEC;

    com_util_mutex_unlock(&handle->reconnect_lock);
    return 0;
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
        com_util_mutex_destroy(&handle->reconnect_lock);
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

    com_util_mutex_lock(&handle->reconnect_lock);
    free(handle->ident);
    handle->ident = dup;
    com_util_mutex_unlock(&handle->reconnect_lock);

    return 0;
}

/* doxygen コメントは、ヘッダに記載 */
void com_util_syslog_sink_dispose_on_unload(com_util_syslog_sink_t *handle)
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
        com_util_mutex_destroy(&handle->reconnect_lock);
    }
    free(handle->ident);
    free(handle);
}

#elif defined(PLATFORM_WINDOWS) && defined(COMPILER_MSVC)
    #pragma warning(disable : 4206)
#endif
