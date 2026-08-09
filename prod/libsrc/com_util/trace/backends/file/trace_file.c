/**
 *******************************************************************************
 *  @file           trace_file.c
 *  @brief          ファイルへトレースを出力するプロバイダーを実装します。
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

#include <com_util/base/result.h>
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

#include <com_util/trace/trace_common.h>
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

/** プロセス内 sink レジストリの初期容量。 */
#define SINK_REGISTRY_INITIAL_CAPACITY 4

/* ===== 内部構造体 ===== */

/**
 *  @brief  ファイル トレース プロバイダー ハンドル構造体 (内部定義) です。
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
    /** 保持する旧世代数。 */
    int generations;
    /** 共有モードの場合 1 (COM_UTIL_TRACE_FILE_SINK_SHARED 指定時)。 */
    int shared;
    /** mutex が初期化済みかどうかのフラグ。 */
    int mutex_initialized;
    /** self_id が有効かどうかのフラグ。 */
    int self_id_valid;
};

/* ===== プロセス内 sink レジストリ ===== */

/**
 *  @brief  プロセス内で同一パスの sink を共有するためのレジストリ エントリです。
 */
struct sink_registry_entry
{
    /** 正規化済みパス文字列 (ヒープ確保)。 */
    char *key;
    /** 共有対象の sink。 */
    com_util_trace_file_sink *sink;
    /** 参照カウント。0 になったら解放する。 */
    int refcount;
#if defined(ARCH_X64)
    /** 明示的アラインメント。 */
    unsigned int pad;
#endif /* ARCH_X64 */
};

struct sink_registry
{
    struct sink_registry_entry *items;
    size_t count;
    size_t capacity;
};

static struct sink_registry s_sink_registry = {0};
static com_util_local_lock *s_sink_registry_lock;
static com_util_once_flag s_sink_registry_lock_once = {0};

static void init_sink_registry_lock(void)
{
    (void)com_util_local_lock_create(&s_sink_registry_lock);
}

/**
 *  @brief  sink レジストリの排他ロックを取得します。
 */
static void sink_registry_lock(void)
{
    com_util_call_once(&s_sink_registry_lock_once, init_sink_registry_lock);
    com_util_local_lock_lock(s_sink_registry_lock, COM_UTIL_SYNC_WAIT_FOREVER);
}

/**
 *  @brief  sink レジストリの排他ロックを解放します。
 */
static void sink_registry_unlock(void)
{
    com_util_local_lock_unlock(s_sink_registry_lock);
}

/**
 *  @brief          レジストリ キー用にパスを正規化した文字列を確保します。
 *  @param[in]      path  対象のファイル パス。
 *  @return         ヒープ確保された正規化済みパス。呼び出し元が free すること。失敗時 NULL。
 *
 *  com_util_path_get_full で絶対化する。絶対化に失敗した場合は元のパス文字列をそのまま使用します。
 */
static char *build_registry_key(const char *path)
{
    char full[PLATFORM_PATH_MAX];
    const char *src = path;
    char *key;
    size_t len;

    if (com_util_path_get_full(full, sizeof(full), NULL, path) == COM_UTIL_OK)
    {
        src = full;
    }

    len = strlen(src);
    key = (char *)malloc(len + 1);
    if (key == NULL)
    {
        return NULL;
    }
    memcpy(key, src, len + 1);
    return key;
}

/**
 *  @brief          レジストリ キー同士を比較します。
 *  @param[in]      lhs  比較する 1 つ目のキー。
 *  @param[in]      rhs  比較する 2 つ目のキー。
 *  @return         一致時 1、不一致時 0。
 *
 *  Windows ではファイル システムの慣習に合わせて大文字小文字を区別しません。
 */
static int registry_key_equals(const char *lhs, const char *rhs)
{
#if defined(PLATFORM_WINDOWS)
    return _stricmp(lhs, rhs) == 0;
#else  /* PLATFORM_WINDOWS */
    return strcmp(lhs, rhs) == 0;
#endif /* PLATFORM_WINDOWS */
}

/**
 *  @brief          キーが一致するレジストリ エントリを検索する (ロック保持中) です。
 *  @param[in]      key  検索する正規化済みパス。
 *  @return         一致したエントリ。見つからない場合 NULL。
 */
static struct sink_registry_entry *sink_registry_find_by_key_locked(const char *key)
{
    size_t i;

    for (i = 0; i < s_sink_registry.count; i++)
    {
        if (registry_key_equals(s_sink_registry.items[i].key, key))
        {
            return &s_sink_registry.items[i];
        }
    }
    return NULL;
}

/**
 *  @brief          sink ポインターが一致するレジストリ エントリを検索する (ロック保持中) です。
 *  @param[in]      sink  検索する sink。
 *  @return         一致したエントリ。見つからない場合 NULL。
 */
static struct sink_registry_entry *sink_registry_find_by_sink_locked(const com_util_trace_file_sink *sink)
{
    size_t i;

    for (i = 0; i < s_sink_registry.count; i++)
    {
        if (s_sink_registry.items[i].sink == sink)
        {
            return &s_sink_registry.items[i];
        }
    }
    return NULL;
}

/**
 *  @brief          sink をレジストリへ登録する (ロック保持中) です。
 *  @param[in]      key   正規化済みパス。成功時はレジストリが所有権を持つ。
 *  @param[in]      sink  登録する sink。
 *  @return         成功時 0、メモリ確保失敗時 -1。
 */
static int sink_registry_register_locked(char *key, com_util_trace_file_sink *sink)
{
    if (s_sink_registry.count == s_sink_registry.capacity)
    {
        struct sink_registry_entry *new_items;
        size_t new_capacity;

        if (s_sink_registry.capacity == 0)
        {
            new_capacity = SINK_REGISTRY_INITIAL_CAPACITY;
        }
        else
        {
            new_capacity = s_sink_registry.capacity * 2;
        }

        new_items = (struct sink_registry_entry *)realloc(s_sink_registry.items,
                                                          new_capacity * sizeof(struct sink_registry_entry));
        if (new_items == NULL)
        {
            return -1;
        }
        s_sink_registry.items = new_items;
        s_sink_registry.capacity = new_capacity;
    }

    s_sink_registry.items[s_sink_registry.count].key = key;
    s_sink_registry.items[s_sink_registry.count].sink = sink;
    s_sink_registry.items[s_sink_registry.count].refcount = 1;
#if defined(ARCH_X64)
    s_sink_registry.items[s_sink_registry.count].pad = 0;
#endif /* ARCH_X64 */
    s_sink_registry.count++;
    return 0;
}

/**
 *  @brief          レジストリ エントリを削除する (ロック保持中)。key の解放は呼び出し元が行います。
 *  @param[in]      entry  削除するエントリ (レジストリ配列内を指すこと)。
 */
static void sink_registry_remove_locked(struct sink_registry_entry *entry)
{
    s_sink_registry.count--;
    *entry = s_sink_registry.items[s_sink_registry.count];
    s_sink_registry.items[s_sink_registry.count].key = NULL;
    s_sink_registry.items[s_sink_registry.count].sink = NULL;
    s_sink_registry.items[s_sink_registry.count].refcount = 0;
#if defined(ARCH_X64)
    s_sink_registry.items[s_sink_registry.count].pad = 0;
#endif /* ARCH_X64 */
}

/* ===== 内部ヘルパー関数 ===== */

/**
 *  @brief  基本オープン フラグを返します。
 *
 *  `com_util_file_open()` は常に他プロセスからの読み取り/書き込み/削除を許可するため
 *  (Linux の open() と対称にする設計。file.h 参照)、単一プロセス モードであっても
 *  Windows の排他書き込みによる複数プロセス誤書き込み防止は働きません。\n
 *  複数プロセスからの同時書き込みを避けたい場合は、呼び出し側で
 *  `com_util_interprocess_lock`/`com_util_interprocess_rwlock` (`sync.h`) による
 *  明示的な排他制御を行ってください。
 */
static int base_open_flags(void)
{
    return COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_APPEND | COM_UTIL_FILE_OPEN_WRITE_THROUGH;
}

/**
 *  @brief  ファイルを追記モードで 1 回だけ開き current_bytes を初期サイズで初期化します。
 *  @param[in,out]  p  ファイル sink。
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

    /* 親ディレクトリを抽出し、存在しない場合は再帰生成する (best-effort) */
    if (com_util_path_dirname(dir, sizeof(dir), NULL, p->path) == COM_UTIL_OK && strcmp(dir, ".") != 0)
    {
        (void)com_util_makedirs(dir, NULL);
    }

    p->self_id_valid = 0;

    if (com_util_file_open(&p->file, p->path, base_open_flags(), NULL) != COM_UTIL_OK)
    {
        p->current_bytes = 0;
        return -1;
    }

    if (com_util_file_get_size(&p->file, &p->current_bytes, NULL) != COM_UTIL_OK)
    {
        p->current_bytes = 0;
    }

    if (p->shared != 0)
    {
        if (com_util_file_get_id(&p->file, &p->self_id, NULL) == COM_UTIL_OK)
        {
            p->self_id_valid = 1;
        }
    }

    return 0;
}

/**
 *  @brief  ファイルを追記モードで開き、失敗時は一定間隔で再試行します。
 *  @param[in,out]  p  ファイル sink。
 *  @return 成功 0 / 失敗 -1。
 *
 *  sink の生成中だけ使用し、生成後のローカル mutex を保持した処理からは呼び出しません。
 */
static int open_file_with_retry(com_util_trace_file_sink *p)
{
    int retry_count;

    if (open_file(p) == 0)
    {
        return 0;
    }

    for (retry_count = 0; retry_count < TRACE_FILE_OPEN_RETRY_COUNT; retry_count++)
    {
        com_util_sleep_ms(TRACE_FILE_OPEN_RETRY_INTERVAL_MS);
        if (open_file(p) == 0)
        {
            return 0;
        }
    }

    return -1;
}

/**
 *  @brief  ローテーション後の新規ファイルを空で作成して 1 回だけ開く (単一プロセス モード用) です。
 *  @param[in,out]  p  ファイル sink。
 *  @return 成功 0 / 失敗 -1。
 *
 *  current_bytes は必ず 0 に設定されます。
 */
static int open_file_truncate(com_util_trace_file_sink *p)
{
    int flags = base_open_flags() | COM_UTIL_FILE_OPEN_TRUNCATE;

    p->current_bytes = 0;

    return com_util_file_open(&p->file, p->path, flags, NULL);
}

/**
 *  @brief  オープン中のファイルが path の現在の実体を指しているか判定する (共有モード用) です。
 *  @return 一致 1 / 不一致または判定不能 0。
 *
 *  他プロセスのローテーションで path がリネームされると、自ハンドルは旧世代を指したままになる。\n
 *  パスの現在の同一性とオープン時にキャッシュした self_id を比較して検出します。
 */
static int sink_points_to_current_file(const com_util_trace_file_sink *p)
{
    com_util_file_id path_id;

    if (p->self_id_valid == 0)
    {
        return 0;
    }

    if (com_util_file_get_path_id(p->path, &path_id, NULL) != COM_UTIL_OK)
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
 *  @brief  開いているファイルを閉じます。未開の場合は何もしません (冪等です)。
 */
static void close_file(com_util_trace_file_sink *p)
{
    (void)com_util_file_close(&p->file, NULL);
}

/**
 *  @brief  トレース ファイルをローテーションします。
 *
 *  ローカル mutex 保持中から呼ばれる。ファイルの再オープンは 1 回だけ試行する。\n
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
    (void)com_util_remove(new_path, NULL);

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

        if (com_util_rename(old_path, new_path, NULL) != 0)
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
 *  @brief  共有モードのローテーション判定と実行を行います。
 *
 *  ローカル mutex 保持中、書き込み成功直後に呼ばれる。\n
 *  全プロセス合計の実サイズ (ハンドル基準) が max_bytes 未満なら何もしない。\n
 *  閾値以上の場合はプロセス間ロックの取得を非ブロッキングで試行し、ロック下で同一性と実サイズを
 *  再確認してからローテーションする。他プロセスがローテーション済みの場合は
 *  開き直すだけにする。\n
 *  プロセス間ロックの取得に失敗した場合はローテーションを見送る
 *  (次回書き込み時に再試行するため、肥大化は一時的に留まる)。
 */
static void check_rotate_shared(com_util_trace_file_sink *p)
{
    size_t real_bytes;

    /* インメモリ集計ではなく実サイズで判定する (複数 writer の合計を反映) */
    if (com_util_file_get_size(&p->file, &real_bytes, NULL) != COM_UTIL_OK)
    {
        return;
    }
    if (real_bytes < p->max_bytes)
    {
        return;
    }

    if (com_util_interprocess_lock_try_lock(p->rotate_lock) != COM_UTIL_OK)
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
        if (com_util_file_get_size(&p->file, &real_bytes, NULL) == COM_UTIL_OK && real_bytes >= p->max_bytes)
        {
            rotate_file(p);
        }
    }

    com_util_interprocess_lock_unlock(p->rotate_lock);
}

/**
 *  @brief  ハンドルが保持する資源を解放します。
 *
 *  create 失敗時と dispose 系の共通処理。\n
 *  未確保 (NULL) のメンバーは何もしません。
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

/**
 *  @brief          新規 sink を生成してファイルを開く (レジストリ登録は行わない) です。
 *  @param[in]      path         出力ファイル パス。
 *  @param[in]      path_len     path のバイト数。
 *  @param[in]      max_bytes    1 ファイルあたりの最大バイト数。0 でデフォルト値を使用。
 *  @param[in]      generations  保持する旧世代数。0 以下でデフォルト値を使用。
 *  @param[in]      flags        動作フラグ。
 *  @return         成功時: ハンドル。失敗時: NULL。
 */
static com_util_trace_file_sink *create_new_sink(const char *path, const size_t path_len, const size_t max_bytes,
                                                 const int generations, const int flags)
{
    com_util_trace_file_sink *handle;

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
    if (com_util_local_lock_create(&handle->mutex) != COM_UTIL_OK)
    {
        free_sink(handle);
        return NULL;
    }
    handle->mutex_initialized = 1;

    /* ファイルを開く; 失敗したらリソースを解放して NULL を返す */
    /* (親ディレクトリの自動生成を含むため、ロック ファイルより先に開く) */
    if (open_file_with_retry(handle) != 0)
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

        if (com_util_interprocess_lock_open(handle->lock_path, &handle->rotate_lock) != COM_UTIL_OK)
        {
            handle->rotate_lock = NULL;
            free_sink(handle);
            return NULL;
        }
    }

    return handle;
}

/* ===== 公開 API ===== */

/* Doxygen コメントは、ヘッダーに記載 */

com_util_trace_file_sink *com_util_trace_file_sink_create(const char *path, const size_t max_bytes,
                                                          const int generations, const int flags)
{
    com_util_trace_file_sink *handle;
    struct sink_registry_entry *entry;
    size_t path_len;
    int requested_shared = 0;
    char *key;

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

    if ((flags & COM_UTIL_TRACE_FILE_SINK_SHARED) != 0)
    {
        requested_shared = 1;
    }

    key = build_registry_key(path);
    if (key == NULL)
    {
        return NULL;
    }

    /* プロセス内で同一パスの sink を共有する (占有モードのプロセス内調停)。
     * 生成自体もロック内で行い、同一パスの並行生成を直列化する。 */
    sink_registry_lock();

    entry = sink_registry_find_by_key_locked(key);
    if (entry != NULL)
    {
        /* 占有モードと共有モードの混在は意味的に矛盾するため拒否する */
        if (entry->sink->shared != requested_shared)
        {
            sink_registry_unlock();
            free(key);
            return NULL;
        }
        entry->refcount++;
        handle = entry->sink;
        sink_registry_unlock();
        free(key);
        return handle;
    }

    handle = create_new_sink(path, path_len, max_bytes, generations, flags);
    if (handle == NULL)
    {
        sink_registry_unlock();
        free(key);
        return NULL;
    }

    if (sink_registry_register_locked(key, handle) != 0)
    {
        sink_registry_unlock();
        free_sink(handle);
        free(key);
        return NULL;
    }

    sink_registry_unlock();
    return handle;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_trace_file_sink_write(com_util_trace_file_sink *handle, const int level,
                                   const com_util_timespec *timestamp, const char *message)
{
    char ts[TRACE_FILE_TS_LEN + 1];
    char buf[TRACE_FILE_LINE_BUF];
    com_util_timespec resolved;
    int fallback_used = 0;
    int len;
    int ret;

    if (handle == NULL || message == NULL)
    {
        return COM_UTIL_OK;
    }

    /* タイムスタンプはロック外で取得する (共有状態へのアクセスなし) */
    if (trace_resolve_timestamp(timestamp, &resolved, &fallback_used) != 0)
    {
        return COM_UTIL_ERR_UNKNOWN;
    }
    if (trace_format_local_timestamp(ts, sizeof(ts), &resolved) != 0)
    {
        return COM_UTIL_ERR_UNKNOWN;
    }

    /* 1 行全体をスタック バッファーへフォーマットする (syscall 回数を最小化) */
    len = snprintf(buf, sizeof(buf), "%s %c %s\n", ts, trace_level_char((com_util_trace_level)level), message);
    if (len <= 0)
    {
        return COM_UTIL_ERR_UNKNOWN;
    }
    if (len >= (int)sizeof(buf))
    {
        /* 切り詰め: バッファー末尾を必ず改行で終端する */
        len = (int)sizeof(buf) - 1;
        buf[len - 1] = '\n';
    }

    /* ロック取得 (タイムアウト付き) */
    if (com_util_local_lock_lock(handle->mutex, FILE_LOCK_TIMEOUT_MS) != COM_UTIL_OK)
    {
        return COM_UTIL_ERR_UNKNOWN;
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
                return COM_UTIL_ERR_UNKNOWN;
            }
        }
    }

    /* ファイルへ書き込む (FILE_FLAG_WRITE_THROUGH / O_DSYNC により自動フラッシュ) */
    ret = com_util_file_write(&handle->file, buf, (size_t)len, NULL);

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
        return COM_UTIL_ERR_UNKNOWN;
    }
    else
    {
        return COM_UTIL_OK;
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_trace_file_sink_dispose(com_util_trace_file_sink *handle)
{
    struct sink_registry_entry *entry;
    char *key_to_free = NULL;
    int should_free = 1;

    if (handle == NULL)
    {
        return;
    }

    sink_registry_lock();
    entry = sink_registry_find_by_sink_locked(handle);
    if (entry != NULL)
    {
        entry->refcount--;
        if (entry->refcount > 0)
        {
            should_free = 0;
        }
        else
        {
            key_to_free = entry->key;
            sink_registry_remove_locked(entry);
        }
    }
    sink_registry_unlock();

    free(key_to_free);
    if (should_free)
    {
        free_sink(handle);
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_trace_file_sink_dispose_on_shutdown(com_util_trace_file_sink *handle)
{
    struct sink_registry_entry *entry;

    if (handle == NULL)
    {
        return;
    }

    /* shutdown 経路ではロックを取得しない (呼び出し側がスレッドの静止を保証する) */
    entry = sink_registry_find_by_sink_locked(handle);
    if (entry != NULL)
    {
        entry->refcount--;
        if (entry->refcount > 0)
        {
            return;
        }
        free(entry->key);
        sink_registry_remove_locked(entry);
    }

    free_sink(handle);
}
