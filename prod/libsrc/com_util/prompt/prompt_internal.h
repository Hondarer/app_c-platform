/**
 *  @file           prompt_internal.h
 *  @brief          プロンプトヘルパー内部定義（非公開）。
 */

#ifndef COM_UTIL_PROMPT_INTERNAL_H
#define COM_UTIL_PROMPT_INTERNAL_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <com_util/prompt/prompt.h>
#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)
    #include <termios.h>
    #include <unistd.h>
    #include <sys/select.h>
#elif defined(PLATFORM_WINDOWS)
    #include <com_util/base/windows_sdk.h>
#endif

/* ---- 1 つの呼び出し元に対応する履歴コンテキスト ---- */
typedef struct
{
    /* ポインタ類を先に並べてパディングを排除 */
    const char *file;       /* __FILE__ の文字列ポインタ（コピー不要） */
    char      **entries;    /* リングバッファ（history_max 個の char*） */
    char       *saved_line; /* ブラウズ前の編集内容退避（LINE_MAX バイト） */
    size_t      count;      /* 有効エントリ数 */
    size_t      head;       /* リング先頭インデックス（最古） */
    int         line;       /* __LINE__ */
    int         browse_idx; /* ブラウズ中インデックス（-1 = 現在行） */
} com_util_prompt_ctx_t;

/* ---- メインハンドル（不透明型の実体） ---- */
/* struct termios (60 バイト) を含むため構造体末尾に 4 バイトのパディングが入る */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"
struct com_util_prompt_t
{
    /* 編集バッファ（readline_at 呼び出しごとに初期化） */
    char   *edit_buf;   /* 編集中の行バッファ（LINE_MAX バイト） */
    size_t  edit_len;   /* 現在の文字数（NUL 除く） */
    size_t  edit_cap;   /* edit_buf の容量 */
    size_t  cursor;     /* カーソル位置（バイトオフセット、0〜edit_len） */

    /* 履歴コンテキスト管理 */
    com_util_prompt_ctx_t *contexts; /* コンテキスト配列（動的拡張） */
    size_t ctx_count;                /* 現在のコンテキスト数 */
    size_t ctx_cap;                  /* contexts 配列の容量 */
    size_t history_max;              /* 各コンテキストの履歴最大数 */

    /* _readline_fmt 用プロンプト文字列バッファ（遅延 malloc、自動拡張） */
    char   *prompt_fmt_buf;
    size_t  prompt_fmt_cap;

    /* TTY 状態 */
    int is_tty;

#if defined(PLATFORM_LINUX)
    struct termios orig_term;
    int            raw_active;
#elif defined(PLATFORM_WINDOWS)
    HANDLE stdin_handle;
    DWORD  orig_in_mode;
    int    raw_active;
#endif
};
#pragma GCC diagnostic pop

/* ---- プラットフォーム抽象インターフェース（各 _platform.c で実装） ---- */

/* stdin が TTY なら 1 */
int prompt_platform_is_tty(void);

/* raw モードに移行 */
void prompt_platform_enter_raw(com_util_prompt_t *p);

/* raw モードを解除して元の設定に戻す */
void prompt_platform_leave_raw(com_util_prompt_t *p);

/* 1 バイトをブロック読み。EOF/エラーは -1 */
int prompt_platform_read_char(com_util_prompt_t *p);

/* 1 バイトを最大 50ms 待って読む。タイムアウト/エラーは -1 */
int prompt_platform_read_char_nb(com_util_prompt_t *p);

#endif /* COM_UTIL_PROMPT_INTERNAL_H */
