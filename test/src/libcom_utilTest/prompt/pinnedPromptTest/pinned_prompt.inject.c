/* テスト対象ソース ファイルの注入用追加ソース
 * このソースはテスト対象ソースの末尾に結合されます
 * この static 関数へのアクセサーによって
 * テスト プログラムからテスト対象ソースの static 関数にアクセスできます
 */
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

void test_pinned_prompt_set_internal_state(com_util_pinned_prompt *screen, int cols, int rows,
                                           int previous_main_bottom_row, int prompt_visible, int status_top_enabled,
                                           int status_bottom_enabled, size_t cursor, size_t view_start)
{
    screen->cols = cols;
    screen->rows = rows;
    screen->prev_main_bottom_row = previous_main_bottom_row;
    screen->prompt_visible = prompt_visible;
    screen->status_top_enabled = status_top_enabled;
    screen->status_bottom_enabled = status_bottom_enabled;
    screen->cursor = cursor;
    screen->view_start = view_start;
}

size_t test_pinned_prompt_view_start(const com_util_pinned_prompt *screen)
{
    return screen->view_start;
}

void test_pinned_prompt_set_mutex_active(com_util_pinned_prompt *screen, int mutex_active)
{
    screen->mutex_active = mutex_active;
}

void test_pinned_prompt_set_input_limits(com_util_pinned_prompt *screen, size_t history_max, size_t input_max_bytes)
{
    screen->history_max = history_max;
    screen->input_max_bytes = input_max_bytes;
}

void test_pinned_prompt_lock_and_unlock(com_util_pinned_prompt *screen)
{
    pinned_prompt_lock(screen);
    pinned_prompt_unlock(screen);
}

int test_pinned_prompt_set_prompt(com_util_pinned_prompt *screen, const char *prompt_string)
{
    return pinned_prompt_set_prompt(screen, prompt_string);
}

void test_pinned_prompt_adjust_view(com_util_pinned_prompt *screen)
{
    pinned_prompt_adjust_view(screen);
}

void test_pinned_prompt_layout(com_util_pinned_prompt *screen, int *prompt_row, int *prompt_separator_row,
                               int *main_bottom_row, int *show_top_status, int *show_bottom_status)
{
    pinned_prompt_layout layout;

    pinned_prompt_calc_layout(screen, &layout);
    *prompt_row = layout.prompt_row;
    *prompt_separator_row = layout.prompt_sep_row;
    *main_bottom_row = layout.main_bottom_row;
    *show_top_status = layout.show_top_status;
    *show_bottom_status = layout.show_bottom_status;
}

void test_pinned_prompt_clear_control_area(com_util_pinned_prompt *screen, int main_bottom_row)
{
    pinned_prompt_layout layout;

    memset(&layout, 0, sizeof(layout));
    layout.main_bottom_row = main_bottom_row;
    pinned_prompt_clear_control_area(screen, &layout);
}

void test_pinned_prompt_hide(com_util_pinned_prompt *screen)
{
    pinned_prompt_hide_prompt_locked(screen);
}

void test_pinned_prompt_finish(com_util_pinned_prompt *screen)
{
    pinned_prompt_finish_prompt_locked(screen);
}

void test_pinned_prompt_cleanup_terminal(com_util_pinned_prompt *screen)
{
    pinned_prompt_cleanup_terminal_locked(screen);
}

int test_pinned_prompt_format(com_util_pinned_prompt *screen, const char *format, ...)
{
    va_list args;
    int result;

    va_start(args, format);
    result = pinned_prompt_format_prompt(screen, format, args);
    va_end(args);
    return result;
}

int test_pinned_prompt_set_status_content(com_util_pinned_prompt *screen, const char *content)
{
    return pinned_prompt_set_status_content(&screen->status_top_left, &screen->status_top_left_cap, content);
}

void test_pinned_prompt_history_null_inputs(com_util_pinned_prompt *screen)
{
    pinned_prompt_history_ctx context;

    memset(&context, 0, sizeof(context));
    pinned_prompt_history_add(screen, NULL, "value");
    pinned_prompt_history_add(screen, &context, NULL);
    pinned_prompt_history_add(screen, &context, "");
    screen->history_max = 0U;
    pinned_prompt_history_add(screen, &context, "value");
    pinned_prompt_clear_edit_line(screen, &context);
    pinned_prompt_backspace(screen);
}

int test_pinned_prompt_history_fill(com_util_pinned_prompt *screen, size_t count)
{
    pinned_prompt_history_ctx *context;
    size_t i;

    context = pinned_prompt_find_or_create_history_ctx(screen, "fill.c", 1);
    if (context == NULL)
    {
        return -1;
    }
    for (i = 0U; i < count; i++)
    {
        pinned_prompt_history_add(screen, context, "value");
        if (context->entries[context->head] != NULL)
        {
            context->entries[context->head][0] = (char)('a' + (int)(i % 26U));
        }
    }
    pinned_prompt_history_prev(screen, context);
    pinned_prompt_history_prev(screen, context);
    pinned_prompt_history_next(screen, context);
    pinned_prompt_history_next(screen, context);
    return 0;
}

int test_pinned_prompt_history_count(const com_util_pinned_prompt *screen)
{
    return (int)screen->history_ctx_count;
}

int test_pinned_prompt_history_failure_state(com_util_pinned_prompt *screen, const char *file, int line)
{
    pinned_prompt_history_ctx *context;

    context = pinned_prompt_find_or_create_history_ctx(screen, file, line);
    if (context == NULL)
    {
        return -1;
    }
    return 0;
}

void test_pinned_prompt_history_null_entry_paths(com_util_pinned_prompt *screen)
{
    pinned_prompt_history_ctx *context;
    char *entry;
    size_t slot;

    context = pinned_prompt_find_or_create_history_ctx(screen, "null-entry.c", 1);
    if (context == NULL)
    {
        return;
    }
    pinned_prompt_history_add(screen, context, "entry");
    slot = PINNED_PROMPT_HIST_IDX(screen, context, context->count - 1U);
    entry = context->entries[slot];
    context->entries[slot] = NULL;
    pinned_prompt_history_prev(screen, context);
    context->browse_idx = 0;
    pinned_prompt_history_next(screen, context);
    context->entries[slot] = entry;
}

void test_pinned_prompt_history_release_entries(com_util_pinned_prompt *screen)
{
    pinned_prompt_history_ctx *context;
    size_t i;

    context = pinned_prompt_find_or_create_history_ctx(screen, "release.c", 1);
    if (context == NULL)
    {
        return;
    }
    for (i = 0U; i < screen->history_max; i++)
    {
        free(context->entries[i]);
    }
    free(context->entries);
    context->entries = NULL;
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

void test_pinned_prompt_set_raw_active(com_util_pinned_prompt *screen, int active)
{
    screen->raw_active = active;
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

void test_pinned_prompt_set_sigwinch_installed(int installed)
{
    s_pinned_sigwinch_installed = installed;
}

void test_pinned_prompt_raise_resize_handler(void)
{
    pinned_prompt_sigwinch_handler(SIGWINCH);
}

int test_pinned_prompt_platform_is_tty(void)
{
    return pinned_prompt_platform_is_tty();
}

#endif /* PLATFORM_LINUX */
