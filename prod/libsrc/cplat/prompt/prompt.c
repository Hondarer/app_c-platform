/**
 *  @file           prompt.c
 *  @brief          プロンプト ヘルパーの共通処理を実装します。
 */

#include <cplat/base/result.h>
#include <cplat/crt/stdlib.h>
#include <cplat/prompt/prompt_internal.h>

#include <cplat/crt/stdio.h>
#include <cplat/crt/string.h>
#include <cplat/crt/unistd.h>
#include <cplat/prompt/prompt_edit.h>

#include <stdarg.h>

#define PROMPT_INPUT_INITIAL_DEFAULT 256U

/* ================================================================
 * 内部キーコード列挙
 * ================================================================ */

typedef enum prompt_key
{
    KEY_CHAR = 0,
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
    KEY_CLEAR,
    KEY_RESIZE,
    KEY_UNKNOWN,
    KEY_EOF,
} prompt_key;

/* ================================================================
 * 履歴ヘルパー
 * ================================================================ */

/* インデックス計算: oldest=0, newest=count-1 */
#define HIST_IDX(ctx, i) (((ctx)->head + (i)) % p->history_max)

static void history_add(cplat_prompt *p, cplat_prompt_ctx *ctx, const char *line)
{
    size_t line_size;
    size_t slot;

    if (line[0] == '\0')
    {
        return;
    }

    /* 直前と同じ行は追加しない */
    if (ctx->count > 0)
    {
        size_t newest_idx = HIST_IDX(ctx, ctx->count - 1);
        if (ctx->entries[newest_idx] != NULL && strcmp(ctx->entries[newest_idx], line) == 0)
        {
            return;
        }
    }
    if (ctx->count == p->history_max)
    {
        /* 最古エントリを上書き */
        cplat_free(ctx->entries[ctx->head]);
        ctx->entries[ctx->head] = NULL;
        ctx->head = (ctx->head + 1) % p->history_max;
    }
    else
    {
        ctx->count++;
    }
    slot = HIST_IDX(ctx, ctx->count - 1);
    line_size = strlen(line) + 1;
    ctx->entries[slot] = (char *)cplat_malloc(line_size);
    if (ctx->entries[slot] != NULL)
    {
        (void)cplat_strcpy(ctx->entries[slot], line_size, line);
    }
}

/* ================================================================
 * 画面再描画
 * ================================================================ */

static void redisplay(const char *prompt_str, const char *buf, size_t len, size_t cursor)
{
    const char *prompt_str_out;
    putchar('\r');
    if (prompt_str)
    {
        prompt_str_out = prompt_str;
    }
    else
    {
        prompt_str_out = "";
    }
    fputs(prompt_str_out, stdout);
    fwrite(buf, 1, len, stdout);
    fputs("\033[0K", stdout); /* 行末クリア */
    if (len > cursor)
    {
        printf("\033[%zuD", len - cursor); /* カーソル位置に戻る */
    }
    fflush(stdout);
}

/* ================================================================
 * エスケープ シーケンス解析
 * ================================================================ */

static prompt_key read_key(cplat_prompt *p, int *ch_out)
{
    int c = prompt_platform_read_char(p);
    if (c == -1)
    {
        return KEY_EOF;
    }
    if (c == -2)
    {
        return KEY_RESIZE;
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
        /* 単独の ESC (次の文字が来なければ) は行消去とする */
        if (c2 == -1)
        {
            return KEY_CLEAR;
        }
        if (c2 == '[')
        {
            int c3 = prompt_platform_read_char_nb(p);
            switch (c3)
            {
            case 'A':
                return KEY_UP;
            case 'B':
                return KEY_DOWN;
            case 'C':
                return KEY_RIGHT;
            case 'D':
                return KEY_LEFT;
            case 'H':
                return KEY_HOME;
            case 'F':
                return KEY_END;
            case '1':
            {
                int c4 = prompt_platform_read_char_nb(p);
                if (c4 == '~')
                {
                    return KEY_HOME;
                }
                else
                {
                    return KEY_UNKNOWN;
                }
            }
            case '3':
            {
                int c4 = prompt_platform_read_char_nb(p);
                if (c4 == '~')
                {
                    return KEY_DELETE;
                }
                else
                {
                    return KEY_UNKNOWN;
                }
            }
            case '4':
            {
                int c4 = prompt_platform_read_char_nb(p);
                if (c4 == '~')
                {
                    return KEY_END;
                }
                else
                {
                    return KEY_UNKNOWN;
                }
            }
            default:
                return KEY_UNKNOWN;
            }
        }
        return KEY_UNKNOWN;
    }
    /* 通常文字 (ASCII 印字可能 + UTF-8 マルチバイト先頭バイト) */
    if (c >= 0x20)
    {
        *ch_out = c;
        return KEY_CHAR;
    }
    return KEY_UNKNOWN;
}

/* ================================================================
 * 履歴ブラウズ
 * ================================================================ */

static void history_browse_prev(cplat_prompt *p, cplat_prompt_ctx *ctx, const char *prompt_str)
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
    if (cplat_prompt_edit_ensure_capacity(&p->edit_buf, &p->edit_cap, p->input_max_bytes, len + 1U) != 0)
    {
        len = p->edit_cap - 1U;
    }
    memcpy(p->edit_buf, entry, len);
    p->edit_buf[len] = '\0';
    p->edit_len = len;
    p->cursor = len;
    redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);
}

static void history_browse_next(cplat_prompt *p, cplat_prompt_ctx *ctx, const char *prompt_str)
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
        memcpy(p->edit_buf, entry, len);
        p->edit_buf[len] = '\0';
        p->edit_len = len;
        p->cursor = len;
    }
    else
    {
        /* 保存した現在行に戻る */
        size_t len = strlen(ctx->saved_line);
        if (cplat_prompt_edit_ensure_capacity(&p->edit_buf, &p->edit_cap, p->input_max_bytes, len + 1U) != 0)
        {
            len = p->edit_cap - 1U;
        }
        memcpy(p->edit_buf, ctx->saved_line, len);
        p->edit_buf[len] = '\0';
        p->edit_len = len;
        p->cursor = len;
        ctx->browse_idx = -1;
    }
    redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);
}

/* ================================================================
 * コンテキスト管理
 * ================================================================ */

static cplat_prompt_ctx *find_or_create_ctx(cplat_prompt *p, const char *file, int line)
{
    size_t i;
    cplat_prompt_ctx *ctx;

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
        size_t new_cap;
        cplat_prompt_ctx *new_contexts;
        if (p->ctx_cap)
        {
            new_cap = p->ctx_cap * 2;
        }
        else
        {
            new_cap = 4;
        }
        new_contexts = (cplat_prompt_ctx *)cplat_realloc(p->contexts, new_cap, sizeof(cplat_prompt_ctx));
        if (new_contexts == NULL)
        {
            return NULL;
        }
        p->contexts = new_contexts;
        p->ctx_cap = new_cap;
    }

    ctx = &p->contexts[p->ctx_count++];
    memset(ctx, 0, sizeof(*ctx));
    ctx->file = file;
    ctx->line = line;
    ctx->entries = (char **)cplat_calloc(p->history_max, sizeof(char *));
    ctx->saved_line = (char *)cplat_malloc(p->input_max_bytes);
    ctx->browse_idx = -1;

    if (ctx->entries == NULL || ctx->saved_line == NULL)
    {
        cplat_free(ctx->entries);
        cplat_free(ctx->saved_line);
        ctx->entries = NULL;
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

/* Doxygen コメントは、ヘッダーに記載 */

cplat_prompt *cplat_prompt_create(const cplat_prompt_options *options)
{
    cplat_prompt *p = (cplat_prompt *)cplat_calloc(1, sizeof(*p));
    size_t history_max;
    size_t input_initial_capacity;
    size_t input_max_bytes;
    size_t opt_history_max;
    size_t opt_input_initial_capacity;
    size_t opt_input_max_bytes;

    if (p == NULL)
    {
        return NULL;
    }

    if (options != NULL)
    {
        opt_history_max = options->history_max;
        opt_input_initial_capacity = options->input_initial_capacity;
        opt_input_max_bytes = options->input_max_bytes;
    }
    else
    {
        opt_history_max = 0U;
        opt_input_initial_capacity = 0U;
        opt_input_max_bytes = 0U;
    }
    cplat_prompt_edit_resolve_options(opt_history_max, opt_input_initial_capacity, opt_input_max_bytes,
                                         PROMPT_INPUT_INITIAL_DEFAULT, &history_max, &input_initial_capacity,
                                         &input_max_bytes);

    p->history_max = history_max;
    p->input_max_bytes = input_max_bytes;
    p->edit_cap = input_initial_capacity;
    p->edit_buf = (char *)cplat_malloc(p->edit_cap);
    if (p->edit_buf == NULL)
    {
        cplat_free(p);
        return NULL;
    }
    p->edit_buf[0] = '\0';
    p->is_tty = cplat_isatty(CPLAT_STREAM_STDIN);
    return p;
}

/* Doxygen コメントは、ヘッダーに記載 */

void cplat_prompt_dispose(cplat_prompt *prompt)
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
        cplat_prompt_ctx *ctx = &prompt->contexts[i];
        for (j = 0; j < prompt->history_max; j++)
        {
            cplat_free(ctx->entries[j]);
        }
        cplat_free(ctx->entries);
        cplat_free(ctx->saved_line);
    }
    cplat_free(prompt->contexts);
    cplat_free(prompt->edit_buf);
    cplat_free(prompt->prompt_fmt_buf);
    cplat_free(prompt);
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_prompt_readline_at(cplat_prompt *p, char *buf, size_t buf_size, const char *prompt_str,
                                const char *file, int line)
{
    cplat_prompt_ctx *ctx;

    if (p == NULL || buf == NULL || buf_size == 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
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
            return CPLAT_ERR_EOF;
        }
        buf[strcspn(buf, "\r\n")] = '\0';
        return CPLAT_OK;
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
            return CPLAT_ERR_EOF;
        }
        buf[strcspn(buf, "\r\n")] = '\0';
        return CPLAT_OK;
    }

    /* raw モードに移行 */
    prompt_platform_enter_raw(p);

    /* 編集バッファー初期化 */
    p->edit_len = 0;
    p->edit_buf[0] = '\0';
    p->cursor = 0;
    ctx->browse_idx = -1;

    redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);

    for (;;)
    {
        int ch = 0;
        prompt_key key = read_key(p, &ch);

        switch (key)
        {
        case KEY_ENTER:
            putchar('\n');
            fflush(stdout);
            {
                size_t copy;
                if (p->edit_len < buf_size - 1)
                {
                    copy = p->edit_len;
                }
                else
                {
                    copy = buf_size - 1;
                }
                memcpy(buf, p->edit_buf, copy);
                buf[copy] = '\0';
            }
            if (p->edit_len > 0)
            {
                history_add(p, ctx, p->edit_buf);
            }
            prompt_platform_leave_raw(p);
            return CPLAT_OK;

        case KEY_EOF:
            putchar('\n');
            fflush(stdout);
            buf[0] = '\0';
            prompt_platform_leave_raw(p);
            return CPLAT_ERR_EOF;

        case KEY_CTRL_C:
            putchar('\n');
            fflush(stdout);
            buf[0] = '\0';
            prompt_platform_leave_raw(p);
            return CPLAT_ERR_CANCELED;

        case KEY_BACKSPACE:
            if (p->cursor > 0)
            {
                size_t prev = cplat_prompt_edit_utf8_prev_boundary(p->edit_buf, p->cursor);
                memmove(p->edit_buf + prev, p->edit_buf + p->cursor, p->edit_len - p->cursor + 1);
                p->edit_len -= p->cursor - prev;
                p->cursor = prev;
                redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);
            }
            break;

        case KEY_DELETE:
            if (p->cursor < p->edit_len)
            {
                size_t next = cplat_prompt_edit_utf8_next_boundary(p->edit_buf, p->edit_len, p->cursor);
                memmove(p->edit_buf + p->cursor, p->edit_buf + next, p->edit_len - next + 1);
                p->edit_len -= next - p->cursor;
                p->edit_buf[p->edit_len] = '\0';
                redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);
            }
            break;

        case KEY_LEFT:
            if (p->cursor > 0)
            {
                p->cursor = cplat_prompt_edit_utf8_prev_boundary(p->edit_buf, p->cursor);
                redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);
            }
            break;

        case KEY_RIGHT:
            if (p->cursor < p->edit_len)
            {
                p->cursor = cplat_prompt_edit_utf8_next_boundary(p->edit_buf, p->edit_len, p->cursor);
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
        case KEY_CLEAR:
            /* Clear current edit line (ESC 単押し) */
            p->edit_len = 0;
            p->edit_buf[0] = '\0';
            p->cursor = 0;
            ctx->browse_idx = -1;
            redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);
            break;

        case KEY_CHAR:
            if (cplat_prompt_edit_ensure_capacity(&p->edit_buf, &p->edit_cap, p->input_max_bytes,
                                                     p->edit_len + 2U) == 0)
            {
                memmove(p->edit_buf + p->cursor + 1, p->edit_buf + p->cursor, p->edit_len - p->cursor + 1);
                p->edit_buf[p->cursor] = (char)ch;
                p->cursor++;
                p->edit_len++;
                redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);
            }
            break;

        case KEY_RESIZE:
            redisplay(prompt_str, p->edit_buf, p->edit_len, p->cursor);
            break;

        case KEY_UNKNOWN:
        default:
            break;
        }
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_prompt_readline_fmt_at(cplat_prompt *p, char *buf, size_t buf_size, const char *file, int line,
                                    const char *fmt, ...)
{
    va_list ap;
    int needed;
    const char *fmt_str;

    if (p == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    /* 遅延初期化 */
    if (p->prompt_fmt_buf == NULL)
    {
        p->prompt_fmt_cap = 256U;
        p->prompt_fmt_buf = (char *)cplat_malloc(p->prompt_fmt_cap);
        if (p->prompt_fmt_buf == NULL)
        {
            return cplat_prompt_readline_at(p, buf, buf_size, "", file, line);
        }
    }

    if (fmt)
    {
        fmt_str = fmt;
    }
    else
    {
        fmt_str = "";
    }
    for (;;)
    {
        va_start(ap, fmt);
        needed = vsnprintf(p->prompt_fmt_buf, p->prompt_fmt_cap, fmt_str, ap); /* 置換対象外: 必要長の照会 */
        va_end(ap);

        if (needed < 0)
        {
            p->prompt_fmt_buf[0] = '\0'; /* エンコード エラー: 空にして続行 */
            break;
        }
        if ((size_t)needed < p->prompt_fmt_cap)
        {
            break; /* 成功 */
        }
        /* バッファー不足: 必要サイズへ拡張して再試行 */
        {
            size_t new_cap = (size_t)needed + 1U;
            char *new_buf = (char *)cplat_realloc(p->prompt_fmt_buf, new_cap, 1U);
            if (new_buf == NULL)
            {
                p->prompt_fmt_buf[p->prompt_fmt_cap - 1U] = '\0'; /* 切り捨てて続行 */
                break;
            }
            p->prompt_fmt_buf = new_buf;
            p->prompt_fmt_cap = new_cap;
        }
    }

    return cplat_prompt_readline_at(p, buf, buf_size, p->prompt_fmt_buf, file, line);
}
