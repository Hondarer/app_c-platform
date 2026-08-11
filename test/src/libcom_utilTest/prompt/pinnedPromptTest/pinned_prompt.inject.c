// pinned_prompt.c の static 関数へテスト用アクセサーを追加します。
#ifndef _IN_TEST_SRC
    #include "pinned_prompt.c"
#endif /* _IN_TEST_SRC */

#include "pinned_prompt.inject.h"

size_t test_pinned_prompt_cstr_len(const char *text)
{
    return cstr_len(text);
}

size_t test_pinned_prompt_utf8_width(const char *text, size_t len, size_t pos)
{
    return utf8_char_display_width(text, len, pos);
}

size_t test_pinned_prompt_ansi_len(const char *text, size_t len, size_t pos)
{
    return ansi_sgr_sequence_len(text, len, pos);
}

size_t test_pinned_prompt_visible_bytes(const char *text, size_t len, size_t start, size_t max_cols)
{
    return pinned_prompt_visible_bytes_from(text, len, start, max_cols);
}

size_t test_pinned_prompt_display_width(const char *text, size_t len, size_t start, size_t end)
{
    return pinned_prompt_display_width_between(text, len, start, end);
}

void test_pinned_prompt_set_tty(com_util_pinned_prompt *screen, int is_tty)
{
    screen->is_tty = is_tty;
}

int test_pinned_prompt_read_key(com_util_pinned_prompt *screen, int *out_ch)
{
    return (int)pinned_prompt_read_key(screen, out_ch);
}

void test_pinned_prompt_set_edit_line(com_util_pinned_prompt *screen, const char *line)
{
    pinned_prompt_set_edit_line(screen, line);
}

void test_pinned_prompt_set_cursor(com_util_pinned_prompt *screen, size_t cursor)
{
    screen->cursor = cursor;
}

void test_pinned_prompt_insert_byte(com_util_pinned_prompt *screen, int ch)
{
    pinned_prompt_insert_byte(screen, ch);
}

void test_pinned_prompt_backspace(com_util_pinned_prompt *screen)
{
    pinned_prompt_backspace(screen);
}

void test_pinned_prompt_delete(com_util_pinned_prompt *screen)
{
    pinned_prompt_delete(screen);
}

const char *test_pinned_prompt_edit_text(const com_util_pinned_prompt *screen)
{
    return screen->edit_buf;
}

size_t test_pinned_prompt_edit_length(const com_util_pinned_prompt *screen)
{
    return screen->edit_len;
}

void test_pinned_prompt_render_state(com_util_pinned_prompt *screen, int is_tty, int prompt_visible,
                                     int status_top_enabled, int status_bottom_enabled, const char *prompt,
                                     const char *edit_line, const char *top_left, const char *top_right,
                                     const char *bottom_left, const char *bottom_right)
{
    screen->is_tty = is_tty;
    screen->prompt_visible = prompt_visible;
    screen->status_top_enabled = status_top_enabled;
    screen->status_bottom_enabled = status_bottom_enabled;
    screen->status_dirty = 1;
    (void)pinned_prompt_set_prompt(screen, prompt);
    (void)pinned_prompt_set_status_content(&screen->status_top_left, &screen->status_top_left_cap, top_left);
    (void)pinned_prompt_set_status_content(&screen->status_top_right, &screen->status_top_right_cap, top_right);
    (void)pinned_prompt_set_status_content(&screen->status_bottom_left, &screen->status_bottom_left_cap, bottom_left);
    (void)pinned_prompt_set_status_content(&screen->status_bottom_right, &screen->status_bottom_right_cap,
                                           bottom_right);
    pinned_prompt_set_edit_line(screen, edit_line);
}

void test_pinned_prompt_render(com_util_pinned_prompt *screen)
{
    pinned_prompt_render_locked(screen);
}

void test_pinned_prompt_history_edge_cases(com_util_pinned_prompt *screen)
{
    pinned_prompt_history_ctx *ctx;

    ctx = pinned_prompt_find_or_create_history_ctx(screen, "edge.c", 1);
    pinned_prompt_history_prev(screen, NULL);
    pinned_prompt_history_next(screen, NULL);
    pinned_prompt_clear_edit_line(screen, NULL);
    pinned_prompt_history_prev(screen, ctx);
    pinned_prompt_history_add(screen, ctx, "same");
    pinned_prompt_history_add(screen, ctx, "same");
    pinned_prompt_history_prev(screen, ctx);
    pinned_prompt_history_prev(screen, ctx);
    pinned_prompt_history_next(screen, ctx);
    pinned_prompt_history_prev(screen, ctx);
}

void test_pinned_prompt_prepare_output(com_util_pinned_prompt *screen)
{
    pinned_prompt_prepare_output_locked(screen);
}

#if defined(PLATFORM_LINUX)

void test_pinned_prompt_reset_platform_state(void)
{
    s_pinned_resize_pending = 0;
    memset(&s_pinned_prev_sigwinch, 0, sizeof(s_pinned_prev_sigwinch));
    s_pinned_sigwinch_installed = 0;
}

void test_pinned_prompt_set_resize_pending(int value)
{
    s_pinned_resize_pending = (sig_atomic_t)value;
}

int test_pinned_prompt_resize_pending(void)
{
    return (int)s_pinned_resize_pending;
}

int test_pinned_prompt_raw_active(const com_util_pinned_prompt *screen)
{
    return screen->raw_active;
}

void test_pinned_prompt_get_size(int *cols, int *rows)
{
    pinned_prompt_platform_get_size(cols, rows);
}

void test_pinned_prompt_enter_raw(com_util_pinned_prompt *screen)
{
    pinned_prompt_platform_enter_raw(screen);
}

void test_pinned_prompt_leave_raw(com_util_pinned_prompt *screen)
{
    pinned_prompt_platform_leave_raw(screen);
}

int test_pinned_prompt_read_char(com_util_pinned_prompt *screen)
{
    return pinned_prompt_platform_read_char(screen);
}

int test_pinned_prompt_read_char_nb(com_util_pinned_prompt *screen)
{
    return pinned_prompt_platform_read_char_nb(screen);
}

#endif /* PLATFORM_LINUX */
