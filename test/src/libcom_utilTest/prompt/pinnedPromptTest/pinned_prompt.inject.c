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
