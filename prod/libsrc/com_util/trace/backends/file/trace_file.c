/**
 *******************************************************************************
 *  @file           trace_file.c
 *  @brief          ファイル トレース プロバイダー実装ファイル。
 *  @author         Tetsuo Honda
 *  @date           2026/04/03
 *  @version        1.0.0
 *
 *  ファイルへのトレース ログ書き込みプロバイダーを提供します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/clock/clock.h>
#include <com_util/crt/file.h>
#include <com_util/crt/path.h>
#include <com_util/crt/stdio.h>
#include <com_util/crt/sys/stat.h>
#include <com_util/sync/sync.h>
#include <com_util/trace/trace_file.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <com_util/trace/backends/file/trace_file_internal.h>

/* ===== 内部定数 ===== */

/** 1 行分のスタック バッファー サイズ。 */
#define TRACE_FILE_LINE_BUF 1100

/** ファイル書き込みロック取得のタイムアウト (ミリ秒)。 */
#define FILE_LOCK_TIMEOUT_MS 100

/** タイムスタンプ部分の文字数 ("YYYY-MM-DDTHH:MM:SS.sss+09:00" = 29 文字)。 */
#define TRACE_FILE_TS_LEN COM_UTIL_CLOCK_ISO8601_LOCAL_MSEC_LEN

/** ローテーション排他用ロック ファイルのサフィックス。 */
#define TRACE_FILE_LOCK_SUFFIX ".lock"

/** パスに付加するサフィックスの最大長 (".lock\0" = 6 文字 >= ".999\0" = 5 文字)。 */
#define TRACE_FILE_SUFFIX_MAX 6

/** ファイル オープン失敗時のリトライ回数。 */
#define TRACE_FILE_OPEN_RETRY_COUNT 3

/** ファイル オープン失敗時のリトライ間隔 (ミリ秒)。 */
#define TRACE_FILE_OPEN_RETRY_INTERVAL_MS 3000

/* ===== 内部構造体 ===== */

/**
 *  @brief  ファイル トレース プロバイダー ハンドル構造体 (内部定義)。
 */
struct com_util_trace_file_sink
{
    /** ヒープ確保済みファイル パス文字列。 */
    char *path;
    /** ローテーション排他用ロック ファイルのパス文字列 (共有モードのみ非 NULL)。 */
    char *lock_path;
    /** ファイル 1 世代あたりの最大バイト数。 */
    size_t max_bytes;
    /** 現ファイルへの書き込み済みバイト数 (単一プロセス モードのインメモリ追跡)。 */
    size_t current_bytes;
    /** スレッド安全のための mutex。 */
    com_util_local_lock *mutex;
    /** ローテーション排他用プロセス間ロック (共有モードのみ非 NULL)。 */
    com_util_interprocess_lock *rotate_lock;
    /** オープン中ファイルの同一性 (共有モードで使用)。 */
    com_util_file_id self_id;
    /** 低レベル ファイル I/O ハンドル。 */
    com_util_file file;
#if defined(PLATFORM_LINUX)
    /** Linux で com_util_file (int) の直後を 8 バイト境界に揃えるためのパディング。 */
    int pad;
#endif /* PLATFORM_LINUX */
    /** 保持する旧世代数。 */
    int generations;
    /** 共有モードの場合 1 (COM_UTIL_TRACE_FILE_SINK_SHARED 指定時)。 */
    int shared;
    /** mutex が初期化済みかどうかのフラグ。 */
    int mutex_initialized;
    /** self_id が有効かどうかのフラグ。 */
    int self_id_valid;
};

/* ===== 内部ヘルパー関数 ===== */

/**
 *  @brief  トレース レベル整数をレベル文字に変換する。
 */
static char level_char(const int level)
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
static int timestamp_is_valid(const com_util_realtime_timestamp *timestamp)
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

/**
 *  @brief  実時刻を "YYYY-MM-DDTHH:MM:SS.sss+09:00" 形式でバッファーへ書き込む。
 *  @param  buf      書き込み先バッファー。
 *  @param  buf_size バッファー サイズ (TRACE_FILE_TS_LEN + 1 以上を推奨)。
 *  @param  resolved 使用する実時刻。
 *  @return 成功 0 / 失敗 -1。
 */
static int format_timestamp(char *buf, const int buf_size, const com_util_realtime_timestamp *resolved)
{
    if (!timestamp_is_valid(resolved))
    {
        return -1;
    }

    return com_util_format_realtime_iso8601_local(buf, (size_t)buf_size, resolved->tv_sec, resolved->tv_nsec);
}

#if defined(PLATFORM_WINDOWS)
static void normalize_path_sep_for_parent(char *path)
{
    char *p;

    for (p = path; *p != '\0'; p++)
    {
        if (*p == '\\')
        {
            *p = PLATFORM_PATH_SEP_CHR;
        }
    }
}
#endif /* PLATFORM_WINDOWS */

/**
 *  @brief  モードに応じた基本オープン フラグを返す。
 *
 *  単一プロセス モードでは共有書き込みを許可しない (Windows では OS が単一 writer を強制する)。\n
 *  SHARE_READ と SHARE_DELETE (外部のログ整理ツール向け) は両モードで許可する。
 */
static int base_open_flags(const com_util_trace_file_sink *p)
{
    int flags = COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_APPEND | COM_UTIL_FILE_OPEN_WRITE_THROUGH |
                COM_UTIL_FILE_OPEN_SHARE_READ | COM_UTIL_FILE_OPEN_SHARE_DELETE;

    if (p->shared != 0)
    {
        flags |= COM_UTIL_FILE_OPEN_SHARE_WRITE;
    }

    return flags;
}

/**
 *  @brief  ファイルを開く。失敗時は一定間隔で再試行する。
 *  @return 成功 0 / 失敗 -1。
 */
static int open_trace_file_with_retry(com_util_file *file, const char *path, const int flags)
{
    int retry_count;

    if (com_util_file_open(file, path, flags) == 0)
    {
        return 0;
    }

    for (retry_count = 0; retry_count < TRACE_FILE_OPEN_RETRY_COUNT; retry_count++)
    {
        com_util_sleep_ms(TRACE_FILE_OPEN_RETRY_INTERVAL_MS);
        if (com_util_file_open(file, path, flags) == 0)
        {
            return 0;
        }
    }

    return -1;
}

/**
 *  @brief  ファイルを追記モードで開き current_bytes を初期サイズで初期化する。
 *  @return 成功 0 / 失敗 -1。
 *
 *  親ディレクトリが存在しない場合は com_util_makedirs で自動生成する (best-effort)。\n
 *  生成に失敗しても後続の com_util_file_open の結果で最終判定する。\n
 *  共有モードではオープンしたファイルの同一性を self_id にキャッシュする
 *  (取得失敗時は self_id_valid = 0 とし、次回書き込み時に開き直す)。
 */
static int open_file(com_util_trace_file_sink *p)
{
    char dir[PLATFORM_PATH_MAX];
    char *sep;

    /* 親ディレクトリを抽出し、存在しない場合は再帰生成する (best-effort) */
    snprintf(dir, sizeof(dir), "%s", p->path);
#if defined(PLATFORM_WINDOWS)
    normalize_path_sep_for_parent(dir);
#endif /* PLATFORM_WINDOWS */
    sep = strrchr(dir, PLATFORM_PATH_SEP_CHR);
    if (sep != NULL)
    {
        *sep = '\0';
        (void)com_util_makedirs(dir);
    }

    p->self_id_valid = 0;

    if (open_trace_file_with_retry(&p->file, p->path, base_open_flags(p)) != 0)
    {
        p->current_bytes = 0;
        return -1;
    }

    if (com_util_file_get_size(&p->file, &p->current_bytes) != 0)
    {
        p->current_bytes = 0;
    }

    if (p->shared != 0)
    {
        if (com_util_file_get_id(&p->file, &p->self_id) == 0)
        {
            p->self_id_valid = 1;
        }
    }

    return 0;
}

/**
 *  @brief  ローテーション後の新規ファイルを空で作成して開く (単一プロセス モード用)。
 *          current_bytes は必ず 0 に設定される。
 *  @return 成功 0 / 失敗 -1。
 */
static int open_file_truncate(com_util_trace_file_sink *p)
{
    int flags = base_open_flags(p) | COM_UTIL_FILE_OPEN_TRUNCATE;

    p->current_bytes = 0;

    return open_trace_file_with_retry(&p->file, p->path, flags);
}

/**
 *  @brief  オープン中のファイルが path の現在の実体を指しているか判定する (共有モード用)。
 *  @return 一致 1 / 不一致または判定不能 0。
 *
 *  他プロセスのローテーションで path がリネームされると、自ハンドルは旧世代を指したままになる。\n
 *  パスの現在の同一性とオープン時にキャッシュした self_id を比較して検出する。
 */
static int sink_points_to_current_file(const com_util_trace_file_sink *p)
{
    com_util_file_id path_id;

    if (p->self_id_valid == 0)
    {
        return 0;
    }

    if (com_util_file_get_path_id(p->path, &path_id) != 0)
    {
        return 0;
    }

    if (path_id.volume == p->self_id.volume && path_id.index == p->self_id.index)
    {
        return 1;
    }

    return 0;
}

/**
 *  @brief  開いているファイルを閉じる。未開の場合は何もしない (冪等)。
 */
static void close_file(com_util_trace_file_sink *p)
{
    com_util_file_close(&p->file);
}

/**
 *  @brief  トレース ファイルをローテーションする。
 *
 *  ロック保持中から呼ばれる。\n
 *  リネームに失敗した場合はその世代でカスケードを打ち切り、
 *  呼び出し元をブロックせずに続行する (ベスト エフォート)。
 */
static void rotate_file(com_util_trace_file_sink *p)
{
    /* パス構築用スタック バッファー */
    char old_path[PLATFORM_PATH_MAX];
    char new_path[PLATFORM_PATH_MAX];
    int gen;

    close_file(p);

    /* 最老世代のファイルを削除する (失敗は無視) */
    snprintf(new_path, sizeof(new_path), "%s.%d", p->path, p->generations);
    (void)com_util_remove(new_path);

    /* path.(gen-1) → path.gen のカスケード リネーム */
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
    if (p->shared != 0)
    {
        /* 共有モード: リネーム直後に他プロセスの writer が path を再作成して
           書き込んでいる場合があるため、切り詰めずに追記モードで開く。 */
        open_file(p);
    }
    else
    {
        open_file_truncate(p);
    }
}

/**
 *  @brief  共有モードのローテーション判定と実行を行う。
 *
 *  ローカル mutex 保持中、書き込み成功直後に呼ばれる。\n
 *  全プロセス合計の実サイズ (ハンドル基準) が max_bytes 未満なら何もしない。\n
 *  閾値以上の場合はプロセス間ロックを取得し、ロック下で同一性と実サイズを
 *  再確認してからローテーションする。他プロセスがローテーション済みの場合は
 *  開き直すだけにする。\n
 *  プロセス間ロックの取得に失敗した場合はローテーションを見送る
 *  (次回書き込み時に再試行するため、肥大化は一時的に留まる)。
 */
static void check_rotate_shared(com_util_trace_file_sink *p)
{
    size_t real_bytes;

    /* インメモリ集計ではなく実サイズで判定する (複数 writer の合計を反映) */
    if (com_util_file_get_size(&p->file, &real_bytes) != 0)
    {
        return;
    }
    if (real_bytes < p->max_bytes)
    {
        return;
    }

    if (com_util_interprocess_lock_lock(p->rotate_lock, FILE_LOCK_TIMEOUT_MS) != COM_UTIL_SYNC_OK)
    {
        return;
    }

    if (sink_points_to_current_file(p) == 0)
    {
        /* 他プロセスが先にローテーション済み: 開き直すだけにする */
        close_file(p);
        open_file(p);
    }
    else
    {
        /* ロック下で実サイズを再確認してからローテーションする */
        if (com_util_file_get_size(&p->file, &real_bytes) == 0 && real_bytes >= p->max_bytes)
        {
            rotate_file(p);
        }
    }

    com_util_interprocess_lock_unlock(p->rotate_lock);
}

/**
 *  @brief  ハンドルが保持する資源を解放する。
 *
 *  create 失敗時と dispose 系の共通処理。\n
 *  未確保 (NULL) のメンバーは何もしない。
 */
static void free_sink(com_util_trace_file_sink *p)
{
    close_file(p);

    if (p->rotate_lock != NULL)
    {
        com_util_interprocess_lock_destroy(p->rotate_lock);
        p->rotate_lock = NULL;
    }

    if (p->mutex_initialized != 0)
    {
        com_util_local_lock_destroy(p->mutex);
        p->mutex_initialized = 0;
    }

    free(p->lock_path);
    free(p->path);
    free(p);
}

/* ===== 公開 API ===== */

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT com_util_trace_file_sink *COM_UTIL_API com_util_trace_file_sink_create(const char *path,
                                                                                       const size_t max_bytes,
                                                                                       const int generations,
                                                                                       const int flags)
{
    com_util_trace_file_sink *handle;
    size_t path_len;

    if (path == NULL || flags < 0)
    {
        return NULL;
    }

    path_len = strlen(path);

    /* パスが長すぎてローテーションやロック ファイルのサフィックスを付加できない場合は拒否する */
    if (path_len + TRACE_FILE_SUFFIX_MAX >= (size_t)PLATFORM_PATH_MAX)
    {
        return NULL;
    }

    handle = (com_util_trace_file_sink *)malloc(sizeof(com_util_trace_file_sink));
    if (handle == NULL)
    {
        return NULL;
    }

    /* 失敗パスで free_sink を使えるよう、先にすべてのメンバーを安全な値にする */
    handle->path = NULL;
    handle->lock_path = NULL;
    handle->mutex = NULL;
    handle->rotate_lock = NULL;
    handle->mutex_initialized = 0;
    handle->self_id_valid = 0;
    handle->current_bytes = 0;
    com_util_file_init(&handle->file);

    handle->shared = 0;
    if ((flags & COM_UTIL_TRACE_FILE_SINK_SHARED) != 0)
    {
        handle->shared = 1;
    }

    if (max_bytes > 0)
    {
        handle->max_bytes = max_bytes;
    }
    else
    {
        handle->max_bytes = COM_UTIL_TRACE_FILE_SINK_DEFAULT_MAX_BYTES;
    }
    if (generations > 0)
    {
        handle->generations = generations;
    }
    else
    {
        handle->generations = COM_UTIL_TRACE_FILE_SINK_DEFAULT_GENERATIONS;
    }

    /* パス文字列をヒープへ複製する */
    handle->path = (char *)malloc(path_len + 1);
    if (handle->path == NULL)
    {
        free_sink(handle);
        return NULL;
    }
    memcpy(handle->path, path, path_len + 1);

    /* 同期プリミティブを初期化する */
    if (com_util_local_lock_create(&handle->mutex) != 0)
    {
        free_sink(handle);
        return NULL;
    }
    handle->mutex_initialized = 1;

    /* ファイルを開く; 失敗したらリソースを解放して NULL を返す */
    /* (親ディレクトリの自動生成を含むため、ロック ファイルより先に開く) */
    if (open_file(handle) != 0)
    {
        free_sink(handle);
        return NULL;
    }

    /* 共有モード: ローテーション排他用のプロセス間ロックを開く */
    if (handle->shared != 0)
    {
        handle->lock_path = (char *)malloc(path_len + sizeof(TRACE_FILE_LOCK_SUFFIX));
        if (handle->lock_path == NULL)
        {
            free_sink(handle);
            return NULL;
        }
        snprintf(handle->lock_path, path_len + sizeof(TRACE_FILE_LOCK_SUFFIX), "%s%s", path, TRACE_FILE_LOCK_SUFFIX);

        if (com_util_interprocess_lock_open(handle->lock_path, &handle->rotate_lock) != COM_UTIL_SYNC_OK)
        {
            handle->rotate_lock = NULL;
            free_sink(handle);
            return NULL;
        }
    }

    return handle;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_trace_file_sink_write(com_util_trace_file_sink *handle, const int level,
                                                                const com_util_realtime_timestamp *timestamp,
                                                                const char *message)
{
    char ts[TRACE_FILE_TS_LEN + 1];
    char buf[TRACE_FILE_LINE_BUF];
    com_util_realtime_timestamp resolved;
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

    /* 1 行全体をスタック バッファーへフォーマットする (syscall 回数を最小化) */
    len = snprintf(buf, sizeof(buf), "%s %c %s\n", ts, level_char(level), message);
    if (len <= 0)
    {
        return -1;
    }
    if (len >= (int)sizeof(buf))
    {
        /* 切り詰め: バッファー末尾を必ず改行で終端する */
        len = (int)sizeof(buf) - 1;
        buf[len - 1] = '\n';
    }

    /* ロック取得 (タイムアウト付き) */
    if (com_util_local_lock_lock(handle->mutex, FILE_LOCK_TIMEOUT_MS) != 0)
    {
        return -1;
    }

    /* 共有モード: 他プロセスのローテーションで path の実体が入れ替わっていたら開き直す */
    if (handle->shared != 0)
    {
        if (sink_points_to_current_file(handle) == 0)
        {
            close_file(handle);
            if (open_file(handle) != 0)
            {
                com_util_local_lock_unlock(handle->mutex);
                return -1;
            }
        }
    }

    /* ファイルへ書き込む (FILE_FLAG_WRITE_THROUGH / O_DSYNC により自動フラッシュ) */
    ret = com_util_file_write(&handle->file, buf, (size_t)len);

    /* 書き込み成功時: サイズを追跡しローテーション閾値を確認する */
    if (ret == 0)
    {
        if (handle->shared != 0)
        {
            check_rotate_shared(handle);
        }
        else
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
    }

    /* ロック解放 */
    com_util_local_lock_unlock(handle->mutex);

    if (ret != 0 || fallback_used)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT void COM_UTIL_API com_util_trace_file_sink_dispose(com_util_trace_file_sink *handle)
{
    if (handle == NULL)
    {
        return;
    }

    free_sink(handle);
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_trace_file_sink_dispose_on_shutdown(com_util_trace_file_sink *handle)
{
    if (handle == NULL)
    {
        return;
    }

    free_sink(handle);
}
