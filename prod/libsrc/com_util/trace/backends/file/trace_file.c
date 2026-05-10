/**
 *******************************************************************************
 *  @file           trace_file.c
 *  @brief          ファイルトレースプロバイダ実装ファイル。
 *  @author         Tetsuo Honda
 *  @date           2026/04/03
 *  @version        1.0.0
 *
 *  ファイルへのトレースログ書き込みプロバイダを提供します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/clock/clock.h>
#include <com_util/crt/file.h>
#include <com_util/crt/path.h>
#include <com_util/crt/stdio.h>
#include <com_util/sync/sync.h>
#include <com_util/trace/trace_file.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <com_util/trace/backends/file/trace_file_internal.h>

/* ===== 内部定数 ===== */

/** 1 行分のスタックバッファサイズ。 */
#define TRACE_FILE_LINE_BUF 1100

/** ファイル書き込みロック取得のタイムアウト (ミリ秒)。 */
#define FILE_LOCK_TIMEOUT_MS 100

/** タイムスタンプ部分の文字数 ("YYYY-MM-DDTHH:MM:SS.sss+09:00" = 29 文字)。 */
#define TRACE_FILE_TS_LEN COM_UTIL_CLOCK_ISO8601_LOCAL_MSEC_LEN

/** ローテーションパスのサフィックス最大長 (".999\0" = 5 文字)。 */
#define TRACE_FILE_SUFFIX_MAX 5

/* ===== 内部構造体 ===== */

/**
 *  @brief  ファイルトレースプロバイダハンドル構造体 (内部定義)。
 */
struct com_util_trace_file_sink
{
    /** ヒープ確保済みファイルパス文字列。 */
    char *path;
    /** ファイル 1 世代あたりの最大バイト数。 */
    size_t max_bytes;
    /** 現ファイルへの書き込み済みバイト数 (インメモリ追跡)。 */
    size_t current_bytes;
    /** 保持する旧世代数。 */
    int generations;
    /** 低レベルファイル I/O ハンドル。 */
    com_util_file_t file;
    /** スレッド安全のための mutex。 */
    com_util_mutex_t *mutex;
    /** mutex が初期化済みかどうかのフラグ。 */
    int mutex_initialized;
    /** 構造体のサイズをアライメント境界に揃えるためのパディング。 */
    int _pad_struct_end;
};

/* ===== 内部ヘルパー関数 ===== */

/**
 *  @brief  トレースレベル整数をレベル文字に変換する。
 */
static char level_char(int level)
{
    switch (level)
    {
    case COM_UTIL_TRACE_LEVEL_CRITICAL:
        return 'C';
    case COM_UTIL_TRACE_LEVEL_ERROR:
        return 'E';
    case COM_UTIL_TRACE_LEVEL_WARNING:
        return 'W';
    case COM_UTIL_TRACE_LEVEL_INFO:
        return 'I';
    case COM_UTIL_TRACE_LEVEL_VERBOSE:
        return 'V';
    default:
        return 'D';
    }
}

/**
 *  @brief  タイムスタンプが有効範囲か判定する。
 */
static int timestamp_is_valid(const com_util_realtime_timestamp_t *timestamp)
{
    return timestamp != NULL && timestamp->tv_nsec >= 0 && timestamp->tv_nsec < 1000000000;
}

/**
 *  @brief  使用するタイムスタンプを解決する。
 *  @param  timestamp      呼び出し側が渡した明示タイムスタンプ。NULL 可。
 *  @param  resolved       解決後のタイムスタンプ格納先。
 *  @param  fallback_used  不正タイムスタンプから現在時刻へ代替した場合 1。
 *  @return 成功 0 / 失敗 -1。
 */
static int resolve_timestamp(const com_util_realtime_timestamp_t *timestamp,
                             com_util_realtime_timestamp_t *resolved,
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
    return timestamp_is_valid(resolved) ? 0 : -1;
}

/**
 *  @brief  実時刻を "YYYY-MM-DDTHH:MM:SS.sss+09:00" 形式でバッファへ書き込む。
 *  @param  buf      書き込み先バッファ。
 *  @param  buf_size バッファサイズ (TRACE_FILE_TS_LEN + 1 以上を推奨)。
 *  @param  resolved 使用する実時刻。
 *  @return 成功 0 / 失敗 -1。
 */
static int format_timestamp(char *buf, int buf_size, const com_util_realtime_timestamp_t *resolved)
{
    if (!timestamp_is_valid(resolved))
    {
        return -1;
    }

    return com_util_format_realtime_iso8601_local(buf, (size_t)buf_size, resolved->tv_sec, resolved->tv_nsec);
}

/**
 *  @brief  ファイルを追記モードで開き current_bytes を初期サイズで初期化する。
 *  @return 成功 0 / 失敗 -1。
 */
static int open_file(com_util_trace_file_sink_t *p)
{
    uint32_t flags = COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_APPEND |
                     COM_UTIL_FILE_OPEN_WRITE_THROUGH | COM_UTIL_FILE_OPEN_SHARE_READ |
                     COM_UTIL_FILE_OPEN_SHARE_DELETE;

    if (com_util_file_open(&p->file, p->path, flags) != 0)
    {
        p->current_bytes = 0;
        return -1;
    }

    if (com_util_file_get_size(&p->file, &p->current_bytes) != 0)
    {
        p->current_bytes = 0;
    }

    return 0;
}

/**
 *  @brief  ローテーション後の新規ファイルを空で作成して開く。
 *          current_bytes は必ず 0 に設定される。
 *  @return 成功 0 / 失敗 -1。
 */
static int open_file_truncate(com_util_trace_file_sink_t *p)
{
    uint32_t flags = COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_TRUNCATE |
                     COM_UTIL_FILE_OPEN_APPEND | COM_UTIL_FILE_OPEN_WRITE_THROUGH |
                     COM_UTIL_FILE_OPEN_SHARE_READ | COM_UTIL_FILE_OPEN_SHARE_DELETE;

    p->current_bytes = 0;

    return com_util_file_open(&p->file, p->path, flags);
}

/**
 *  @brief  開いているファイルを閉じる。未開の場合は何もしない (冪等)。
 */
static void close_file(com_util_trace_file_sink_t *p)
{
    com_util_file_close(&p->file);
}

/**
 *  @brief  トレースファイルをローテーションする。
 *  @details ロック保持中から呼ばれる。\n
 *           リネームに失敗した場合はその世代でカスケードを打ち切り、
 *           呼び出し元をブロックせずに続行する (ベストエフォート)。
 */
static void rotate_file(com_util_trace_file_sink_t *p)
{
    /* パス構築用スタックバッファ */
    char old_path[PLATFORM_PATH_MAX];
    char new_path[PLATFORM_PATH_MAX];
    int gen;

    close_file(p);

    /* 最老世代のファイルを削除する (失敗は無視) */
    snprintf(new_path, sizeof(new_path), "%s.%d", p->path, p->generations);
    (void)com_util_remove(new_path);

    /* path.(gen-1) → path.gen のカスケードリネーム */
    for (gen = p->generations; gen >= 1; gen--)
    {
        /* 移動先: path.gen */
        snprintf(new_path, sizeof(new_path), "%s.%d", p->path, gen);

        /* 移動元: gen==1 のときは path そのもの */
        if (gen == 1)
        {
            /* old_path に path をコピー */
            snprintf(old_path, sizeof(old_path), "%s", p->path);
        }
        else
        {
            snprintf(old_path, sizeof(old_path), "%s.%d", p->path, gen - 1);
        }

        if (com_util_rename(old_path, new_path) != 0)
        {
            /* リネーム失敗: カスケードをここで打ち切る */
            break;
        }
    }

    /* 新規ファイルを作成して開く (失敗しても未オープンのまま続行) */
    open_file_truncate(p);
}

/* ===== 公開 API ===== */

/* doxygen コメントは、ヘッダに記載 */
COM_UTIL_EXPORT com_util_trace_file_sink_t *COM_UTIL_API com_util_trace_file_sink_create(const char *path, size_t max_bytes,
                                                                           int generations)
{
    com_util_trace_file_sink_t *handle;
    size_t path_len;

    if (path == NULL)
    {
        return NULL;
    }

    path_len = strlen(path);

    /* パスが長すぎてローテーションサフィックスを付加できない場合は拒否する */
    if (path_len + TRACE_FILE_SUFFIX_MAX >= (size_t)PLATFORM_PATH_MAX)
    {
        return NULL;
    }

    handle = (com_util_trace_file_sink_t *)malloc(sizeof(com_util_trace_file_sink_t));
    if (handle == NULL)
    {
        return NULL;
    }

    /* パス文字列をヒープへ複製する */
    handle->path = (char *)malloc(path_len + 1);
    if (handle->path == NULL)
    {
        free(handle);
        return NULL;
    }
    memcpy(handle->path, path, path_len + 1);

    handle->max_bytes = (max_bytes > 0) ? max_bytes : COM_UTIL_TRACE_FILE_SINK_DEFAULT_MAX_BYTES;
    handle->generations = (generations > 0) ? generations : COM_UTIL_TRACE_FILE_SINK_DEFAULT_GENERATIONS;
    handle->current_bytes = 0;
    com_util_file_init(&handle->file);

    /* 同期プリミティブを初期化する */
    handle->mutex_initialized = 0;
    if (com_util_mutex_create(&handle->mutex) != 0)
    {
        free(handle->path);
        free(handle);
        return NULL;
    }
    handle->mutex_initialized = 1;

    /* ファイルを開く; 失敗したらリソースを解放して NULL を返す */
    if (open_file(handle) != 0)
    {
        if (handle->mutex_initialized)
        {
            com_util_mutex_destroy(handle->mutex);
        }
        free(handle->path);
        free(handle);
        return NULL;
    }

    return handle;
}

/* doxygen コメントは、ヘッダに記載 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_trace_file_sink_write(com_util_trace_file_sink_t *handle, int level,
                                                                const com_util_realtime_timestamp_t *timestamp,
                                                                const char *message)
{
    char ts[TRACE_FILE_TS_LEN + 1];
    char buf[TRACE_FILE_LINE_BUF];
    com_util_realtime_timestamp_t resolved;
    int fallback_used = 0;
    int len;
    int ret;

    if (handle == NULL || message == NULL)
    {
        return 0;
    }

    /* タイムスタンプはロック外で取得する (共有状態へのアクセスなし) */
    if (resolve_timestamp(timestamp, &resolved, &fallback_used) != 0)
    {
        return -1;
    }
    if (format_timestamp(ts, (int)sizeof(ts), &resolved) != 0)
    {
        return -1;
    }

    /* 1 行全体をスタックバッファへフォーマットする (syscall 回数を最小化) */
    len = snprintf(buf, sizeof(buf), "%s %c %s\n", ts, level_char(level), message);
    if (len <= 0)
    {
        return -1;
    }
    if (len >= (int)sizeof(buf))
    {
        /* 切り詰め: バッファ末尾を必ず改行で終端する */
        len = (int)sizeof(buf) - 1;
        buf[len - 1] = '\n';
    }

    /* ロック取得 (タイムアウト付き) */
    if (com_util_mutex_lock(handle->mutex, FILE_LOCK_TIMEOUT_MS) != 0)
    {
        return -1;
    }

    /* ファイルへ書き込む (FILE_FLAG_WRITE_THROUGH / O_DSYNC により自動フラッシュ) */
    ret = com_util_file_write(&handle->file, buf, (size_t)len);

    /* 書き込み成功時: サイズを追跡しローテーション閾値を確認する */
    if (ret == 0)
    {
        /* size_t のオーバーフローを防ぐ (実用上は発生しないが防御的に扱う) */
        if (handle->current_bytes <= (size_t)-1 - (size_t)len)
        {
            handle->current_bytes += (size_t)len;
        }

        if (handle->current_bytes >= handle->max_bytes)
        {
            rotate_file(handle);
        }
    }

    /* ロック解放 */
    com_util_mutex_unlock(handle->mutex);

    return (ret != 0 || fallback_used) ? -1 : 0;
}

/* doxygen コメントは、ヘッダに記載 */
COM_UTIL_EXPORT void COM_UTIL_API com_util_trace_file_sink_dispose(com_util_trace_file_sink_t *handle)
{
    if (handle == NULL)
    {
        return;
    }

    close_file(handle);

    if (handle->mutex_initialized)
    {
        com_util_mutex_destroy(handle->mutex);
        handle->mutex_initialized = 0;
    }

    free(handle->path);
    free(handle);
}

/* doxygen コメントは、ヘッダに記載 */
void com_util_trace_file_sink_dispose_on_shutdown(com_util_trace_file_sink_t *handle)
{
    if (handle == NULL)
    {
        return;
    }

    close_file(handle);

    if (handle->mutex_initialized)
    {
        com_util_mutex_destroy(handle->mutex);
    }

    free(handle->path);
    free(handle);
}
