/**
 *  @file           prompt.c
 *  @brief          プロンプトヘルパー共通実装。
 */

#include "prompt_internal.h"

#include <com_util/crt/string.h>

#include <stdarg.h>

/* ================================================================
 * 内部キーコード列挙
 * ================================================================ */

typedef enum
{
    KEY_CHAR      = 0,
    KEY_ENTER,
    KEY_BACKSPACE,
    KEY_DELETE,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_UP,
    KEY_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_CTRL_C,
    KEY_UNKNOWN,
    KEY_EOF,
} prompt_key_t;

/* ================================================================
 * 履歴ヘルパー
 * ================================================================ */

/* インデックス計算: oldest=0, newest=count-1 */
#define HIST_IDX(ctx, i) (((ctx)->head + (i)) % p->history_max)

static void history_add(com_util_prompt_t *p,
                        com_util_prompt_ctx_t *ctx,
                        const char *line)
{
    size_t line_size;
    size_t slot;
    /* 直前と同じ行は追加しない */
    if (ctx->count > 0)
    {
        size_t newest_idx = HIST_IDX(ctx, ctx->count - 1);
        if (ctx->entries[newest_idx] != NULL &&
            strcmp(ctx->entries[newest_idx], line) == 0)
        {
            return;
        }
    }
    if (ctx->count == p->history_max)
    {
        /* 最古エントリを上書き */
        free(ctx->entries[ctx->head]);
        ctx->entries[ctx->head] = NULL;
        ctx->head = (ctx->head + 1) % p->history_max;
    }
    else
    {
        ctx->count++;
    }
    slot = HIST_IDX(ctx, ctx->count - 1);
    line_size = strlen(line) + 1;
    ctx->entries[slot] = (char *)malloc(line_size);
    if (ctx->entries[slot] != NULL)
    {
        (void)com_util_strcpy(ctx->entries[slot], line_size, line);
    }
}

/* ================================================================
 * 画面再描画
 * ================================================================ */

static void redisplay(const char *prompt_str,
                      const char *buf, size_t len, size_t cursor)
{
    putchar('\r');
    fputs(prompt_str ? prompt_str : "", stdout);
    fwrite(buf, 1, len, stdout);
    fputs("\033[0K", stdout); /* 行末クリア */
    if (len > cursor)
    {
        printf("\033[%zuD", len - cursor); /* カーソル位置に戻る */
    }
    fflush(stdout);
}

/* ================================================================
 * エスケープシーケンス解析
 * ================================================================ */

static prompt_key_t read_key(com_util_prompt_t *p, int *out_ch)
{
    int c = prompt_platform_read_char(p);
    if (c == -1)
    {
        return KEY_EOF;
    }
    if (c == '\r' || c == '\n')
    {
        return KEY_ENTER;
    }
    if (c == 0x03)
    {
        return KEY_CTRL_C;
    }
    if (c == 0x7F || c == 0x08)
    {
        return KEY_BACKSPACE;
    }
    if (c == 0x1B)
    {
        /* ESC シーケンス */
        int c2 = prompt_platform_read_char_nb(p);
        if (c2 == '[')
        {
            int c3 = prompt_platform_read_char_nb(p);
            switch (c3)
            {
            case 'A': return KEY_UP;
            case 'B': return KEY_DOWN;
            case 'C': return KEY_RIGHT;
            case 'D': return KEY_LEFT;
            case 'H': return KEY_HOME;
            case 'F': return KEY_END;
            case '1':
            {
                int c4 = prompt_platform_read_char_nb(p);
                return (c4 == '~') ? KEY_HOME : KEY_UNKNOWN;
            }
            case '3':
            {
                int c4 = prompt_platform_read_char_nb(p);
                return (c4 == '~') ? KEY_DELETE : KEY_UNKNOWN;
            }
            case '4':
            {
                int c4 = prompt_platform_read_char_nb(p);
                return (c4 == '~') ? KEY_END : KEY_UNKNOWN;
            }
            default:
                return KEY_UNKNOWN;
            }
        }
        return KEY_UNKNOWN;
    }
    /* 通常文字（ASCII 印字可能 + UTF-8 マルチバイト先頭バイト） */
    if (c >= 0x20 || (unsigned char)c >= 0x80)
    {
        *out_ch = c;
        return KEY_CHAR;
    }
    return KEY_UNKNOWN;
}

/* ================================================================
 * 履歴ブラウズ
 * ================================================================ */

static void history_browse_prev(com_util_prompt_t *p,
                                com_util_prompt_ctx_t *ctx,
                                const char *prompt_str)
{
    const char *entry;
    size_t len;
    if (ctx->count == 0)
    {
        return;
    }
    if (ctx->browse_idx == -1)
    {
        /* 現在の編集内容を退避 */
        memcpy(ctx->saved_line, p->edit_buf, p->edit_len + 1);
        ctx->browse_idx = (int)ctx->count - 1; /* 最新エントリから */
    }
    else if (ctx->browse_idx > 0)
    {
        ctx->browse_idx--;
    }
    else
    {
        return; /* 最古まで到達済み */
    }
    entry = ctx->entries[HIST_IDX(ctx, (size_t)ctx->browse_idx)];
    if (entry == NULL)
    {
        return;
    }
    len = strlen(entry);
    if (len >= p->edit_cap)
    {
        len = p->edit_cap - 1;
    }
    memcpy(p->edit_buf, entry, len);
    p->edit_buf[len] = '\0';
    p->edit_len = len;
    p->cursor   = len;
    redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);
}

static void history_browse_next(com_util_prompt_t *p,
                                com_util_prompt_ctx_t *ctx,
                                const char *prompt_str)
{
    if (ctx->browse_idx == -1)
    {
        return;
    }
    if (ctx->browse_idx < (int)ctx->count - 1)
    {
        const char *entry;
        size_t len;
        ctx->browse_idx++;
        entry = ctx->entries[HIST_IDX(ctx, (size_t)ctx->browse_idx)];
        if (entry == NULL)
        {
            return;
        }
        len = strlen(entry);
        if (len >= p->edit_cap)
        {
            len = p->edit_cap - 1;
        }
        memcpy(p->edit_buf, entry, len);
        p->edit_buf[len] = '\0';
        p->edit_len = len;
        p->cursor   = len;
    }
    else
    {
        /* 保存した現在行に戻る */
        size_t len = strlen(ctx->saved_line);
        if (len >= p->edit_cap)
        {
            len = p->edit_cap - 1;
        }
        memcpy(p->edit_buf, ctx->saved_line, len);
        p->edit_buf[len] = '\0';
        p->edit_len      = len;
        p->cursor        = len;
        ctx->browse_idx  = -1;
    }
    redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);
}

/* ================================================================
 * コンテキスト管理
 * ================================================================ */

static com_util_prompt_ctx_t *find_or_create_ctx(com_util_prompt_t *p,
                                                  const char *file,
                                                  int line)
{
    size_t i;
    com_util_prompt_ctx_t *ctx;

    /* 既存コンテキストを検索 */
    for (i = 0; i < p->ctx_count; i++)
    {
        if (p->contexts[i].line == line && p->contexts[i].file == file)
        {
            return &p->contexts[i];
        }
    }

    /* 新規作成 */
    if (p->ctx_count == p->ctx_cap)
    {
        size_t new_cap = p->ctx_cap ? p->ctx_cap * 2 : 4;
        com_util_prompt_ctx_t *new_contexts = (com_util_prompt_ctx_t *)realloc(
            p->contexts, new_cap * sizeof(com_util_prompt_ctx_t));
        if (new_contexts == NULL)
        {
            return NULL;
        }
        p->contexts = new_contexts;
        p->ctx_cap  = new_cap;
    }

    ctx = &p->contexts[p->ctx_count++];
    memset(ctx, 0, sizeof(*ctx));
    ctx->file       = file;
    ctx->line       = line;
    ctx->entries    = (char **)calloc(p->history_max, sizeof(char *));
    ctx->saved_line = (char *)malloc(COM_UTIL_PROMPT_LINE_MAX);
    ctx->browse_idx = -1;

    if (ctx->entries == NULL || ctx->saved_line == NULL)
    {
        free(ctx->entries);
        free(ctx->saved_line);
        ctx->entries    = NULL;
        ctx->saved_line = NULL;
        p->ctx_count--;
        return NULL;
    }
    ctx->saved_line[0] = '\0';
    return ctx;
}

/* ================================================================
 * 公開 API
 * ================================================================ */

com_util_prompt_t *com_util_prompt_create(size_t history_max)
{
    com_util_prompt_t *p = (com_util_prompt_t *)calloc(1, sizeof(*p));
    if (p == NULL)
    {
        return NULL;
    }
    if (history_max == 0)
    {
        history_max = COM_UTIL_PROMPT_HISTORY_DEFAULT;
    }
    p->history_max = history_max;
    p->edit_cap    = COM_UTIL_PROMPT_LINE_MAX;
    p->edit_buf    = (char *)malloc(p->edit_cap);
    if (p->edit_buf == NULL)
    {
        free(p);
        return NULL;
    }
    p->edit_buf[0] = '\0';
    p->is_tty = prompt_platform_is_tty();
    return p;
}

void com_util_prompt_dispose(com_util_prompt_t *prompt)
{
    size_t i;
    if (prompt == NULL)
    {
        return;
    }
    /* raw モード中なら復元 */
    prompt_platform_leave_raw(prompt);

    /* 全コンテキストのリソースを解放 */
    for (i = 0; i < prompt->ctx_count; i++)
    {
        size_t j;
        com_util_prompt_ctx_t *ctx = &prompt->contexts[i];
        for (j = 0; j < prompt->history_max; j++)
        {
            free(ctx->entries[j]);
        }
        free(ctx->entries);
        free(ctx->saved_line);
    }
    free(prompt->contexts);
    free(prompt->edit_buf);
    free(prompt->prompt_fmt_buf);
    free(prompt);
}

int _com_util_prompt_readline(com_util_prompt_t *p,
                               char              *buf,
                               size_t             buf_size,
                               const char        *prompt_str,
                               const char        *file,
                               int                line)
{
    com_util_prompt_ctx_t *ctx;

    if (p == NULL || buf == NULL || buf_size == 0)
    {
        return 0;
    }
    buf[0] = '\0';

    /* TTY でなければ fgets() フォールバック */
    if (!p->is_tty)
    {
        if (prompt_str != NULL)
        {
            fputs(prompt_str, stdout);
            fflush(stdout);
        }
        if (fgets(buf, (int)buf_size, stdin) == NULL)
        {
            return 0;
        }
        buf[strcspn(buf, "\r\n")] = '\0';
        return 1;
    }

    /* 呼び出し元に対応するコンテキストを取得 */
    ctx = find_or_create_ctx(p, file, line);
    if (ctx == NULL)
    {
        /* コンテキスト取得失敗時は fgets() フォールバック */
        if (prompt_str != NULL)
        {
            fputs(prompt_str, stdout);
            fflush(stdout);
        }
        if (fgets(buf, (int)buf_size, stdin) == NULL)
        {
            return 0;
        }
        buf[strcspn(buf, "\r\n")] = '\0';
        return 1;
    }

    /* raw モードに移行 */
    prompt_platform_enter_raw(p);

    /* 編集バッファ初期化 */
    p->edit_len     = 0;
    p->edit_buf[0]  = '\0';
    p->cursor       = 0;
    ctx->browse_idx = -1;

    redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);

    for (;;)
    {
        int ch = 0;
        prompt_key_t key = read_key(p, &ch);

        switch (key)
        {
        case KEY_ENTER:
            putchar('\n');
            fflush(stdout);
            {
                size_t copy = (p->edit_len < buf_size - 1) ? p->edit_len : buf_size - 1;
                memcpy(buf, p->edit_buf, copy);
                buf[copy] = '\0';
            }
            if (p->edit_len > 0)
            {
                history_add(p, ctx, p->edit_buf);
            }
            prompt_platform_leave_raw(p);
            return 1;

        case KEY_EOF:
        case KEY_CTRL_C:
            putchar('\n');
            fflush(stdout);
            buf[0] = '\0';
            prompt_platform_leave_raw(p);
            return 0;

        case KEY_BACKSPACE:
            if (p->cursor > 0)
            {
                memmove(p->edit_buf + p->cursor - 1,
                        p->edit_buf + p->cursor,
                        p->edit_len - p->cursor + 1);
                p->cursor--;
                p->edit_len--;
                redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);
            }
            break;

        case KEY_DELETE:
            if (p->cursor < p->edit_len)
            {
                memmove(p->edit_buf + p->cursor,
                        p->edit_buf + p->cursor + 1,
                        p->edit_len - p->cursor);
                p->edit_len--;
                p->edit_buf[p->edit_len] = '\0';
                redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);
            }
            break;

        case KEY_LEFT:
            if (p->cursor > 0)
            {
                p->cursor--;
                redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);
            }
            break;

        case KEY_RIGHT:
            if (p->cursor < p->edit_len)
            {
                p->cursor++;
                redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);
            }
            break;

        case KEY_HOME:
            if (p->cursor != 0)
            {
                p->cursor = 0;
                redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);
            }
            break;

        case KEY_END:
            if (p->cursor != p->edit_len)
            {
                p->cursor = p->edit_len;
                redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);
            }
            break;

        case KEY_UP:
            history_browse_prev(p, ctx, prompt_str);
            break;

        case KEY_DOWN:
            history_browse_next(p, ctx, prompt_str);
            break;

        case KEY_CHAR:
            if (p->edit_len + 1 < p->edit_cap)
            {
                memmove(p->edit_buf + p->cursor + 1,
                        p->edit_buf + p->cursor,
                        p->edit_len - p->cursor + 1);
                p->edit_buf[p->cursor] = (char)ch;
                p->cursor++;
                p->edit_len++;
                redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);
            }
            break;

        case KEY_UNKNOWN:
        default:
            break;
        }
    }
}

int _com_util_prompt_readline_fmt(com_util_prompt_t *p,
                                   char              *buf,
                                   size_t             buf_size,
                                   const char        *file,
                                   int                line,
                                   const char        *fmt,
                                   ...)
{
    va_list ap;
    int needed;

    if (p == NULL)
    {
        return 0;
    }

    /* 遅延初期化 */
    if (p->prompt_fmt_buf == NULL)
    {
        p->prompt_fmt_cap = 256U;
        p->prompt_fmt_buf = (char *)malloc(p->prompt_fmt_cap);
        if (p->prompt_fmt_buf == NULL)
        {
            return _com_util_prompt_readline(p, buf, buf_size, "", file, line);
        }
    }

    for (;;)
    {
        va_start(ap, fmt);
        needed = vsnprintf(p->prompt_fmt_buf, p->prompt_fmt_cap,
                           fmt ? fmt : "", ap);
        va_end(ap);

        if (needed < 0)
        {
            p->prompt_fmt_buf[0] = '\0'; /* エンコードエラー: 空にして続行 */
            break;
        }
        if ((size_t)needed < p->prompt_fmt_cap)
        {
            break; /* 成功 */
        }
        /* バッファ不足: 必要サイズへ拡張して再試行 */
        {
            size_t new_cap = (size_t)needed + 1U;
            char  *new_buf = (char *)realloc(p->prompt_fmt_buf, new_cap);
            if (new_buf == NULL)
            {
                p->prompt_fmt_buf[p->prompt_fmt_cap - 1U] = '\0'; /* 切り捨てて続行 */
                break;
            }
            p->prompt_fmt_buf = new_buf;
            p->prompt_fmt_cap = new_cap;
        }
    }

    return _com_util_prompt_readline(p, buf, buf_size,
                                      p->prompt_fmt_buf, file, line);
}
