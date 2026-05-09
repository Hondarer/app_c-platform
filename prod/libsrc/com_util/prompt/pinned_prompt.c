/**
 *  @file           pinned_prompt.c
 *  @brief          Pinned prompt implementation.
 */

#include <com_util/prompt/pinned_prompt.h>

#include <com_util/console/console.h>
#include <com_util/crt/string.h>
#include <com_util/sync/sync.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(PLATFORM_LINUX)
    #include <errno.h>
    #include <sys/ioctl.h>
    #include <sys/select.h>
    #include <termios.h>
    #include <unistd.h>
#elif defined(PLATFORM_WINDOWS)
    #include <com_util/base/windows_sdk.h>
#endif

typedef enum
{
    PINNED_PROMPT_KEY_CHAR = 0,
    PINNED_PROMPT_KEY_ENTER,
    PINNED_PROMPT_KEY_BACKSPACE,
    PINNED_PROMPT_KEY_DELETE,
    PINNED_PROMPT_KEY_LEFT,
    PINNED_PROMPT_KEY_RIGHT,
    PINNED_PROMPT_KEY_UP,
    PINNED_PROMPT_KEY_DOWN,
    PINNED_PROMPT_KEY_HOME,
    PINNED_PROMPT_KEY_END,
    PINNED_PROMPT_KEY_CTRL_C,
    PINNED_PROMPT_KEY_UNKNOWN,
    PINNED_PROMPT_KEY_EOF
} pinned_prompt_key_t;

typedef struct
{
    int prompt_row;
    int top_status_row;
    int prompt_sep_row;
    int bottom_sep_row;
    int bottom_status_row;
    int main_bottom_row;
    int show_top_status;
    int show_bottom_status;
} pinned_prompt_layout_t;

struct com_util_pinned_prompt_t
{
    com_util_mutex_t mutex;

    char   *edit_buf;
    char  **history;
    char   *saved_line;
    char   *prompt_buf;
    char   *fmt_buf;

    char   *status_top_left;
    char   *status_top_right;
    char   *status_bottom_left;
    char   *status_bottom_right;

    size_t  edit_len;
    size_t  edit_cap;
    size_t  cursor;
    size_t  view_start;
    size_t  history_max;
    size_t  history_count;
    size_t  history_head;
    size_t  prompt_cap;
    size_t  fmt_cap;

    size_t  status_top_left_cap;
    size_t  status_top_right_cap;
    size_t  status_bottom_left_cap;
    size_t  status_bottom_right_cap;

    int mutex_active;
    int browse_idx;
    int is_tty;
    int raw_active;
    int prompt_visible;
    int status_top_enabled;
    int status_bottom_enabled;
    int status_dirty;
    int cols;
    int rows;

#if defined(PLATFORM_LINUX)
    struct termios orig_term;
    char           padding[4];
#elif defined(PLATFORM_WINDOWS)
    HANDLE stdin_handle;
    DWORD  orig_in_mode;
    char   padding[4];
#endif
};

#define PINNED_PROMPT_HIST_IDX(s, i) (((s)->history_head + (i)) % (s)->history_max)

static size_t cstr_len(const char *s)
{
    return s != NULL ? strlen(s) : 0U;
}

static int utf8_is_continuation(unsigned char c)
{
    return (c & 0xC0U) == 0x80U;
}

static size_t utf8_prev_boundary(const char *buf, size_t pos)
{
    if (pos == 0U)
    {
        return 0U;
    }
    pos--;
    while (pos > 0U && utf8_is_continuation((unsigned char)buf[pos]))
    {
        pos--;
    }
    return pos;
}

static size_t utf8_next_boundary(const char *buf, size_t len, size_t pos)
{
    if (pos >= len)
    {
        return len;
    }
    pos++;
    while (pos < len && utf8_is_continuation((unsigned char)buf[pos]))
    {
        pos++;
    }
    return pos;
}

static size_t utf8_sanitize_boundary(const char *buf, size_t len, size_t pos)
{
    if (pos > len)
    {
        pos = len;
    }
    while (pos > 0U && pos < len && utf8_is_continuation((unsigned char)buf[pos]))
    {
        pos--;
    }
    return pos;
}

static size_t utf8_char_display_width(const char *buf, size_t len, size_t pos)
{
    unsigned char first_byte;
    unsigned int code_point;

    if (pos >= len)
        return 0U;

    first_byte = (unsigned char)buf[pos];

    if ((first_byte & 0x80U) == 0U) {
        return 1U;
    }

    if ((first_byte & 0xE0U) == 0xC0U && pos + 1U < len) {
        code_point = ((unsigned int)(first_byte & 0x1FU) << 6U) |
                     (unsigned int)((unsigned char)buf[pos + 1U] & 0x3FU);
        if (code_point >= 0x0300U && code_point <= 0x036FU)
            return 0U;
        return 1U;
    }

    if ((first_byte & 0xF0U) == 0xE0U && pos + 2U < len) {
        code_point = ((unsigned int)(first_byte & 0x0FU) << 12U) |
                     ((unsigned int)((unsigned char)buf[pos + 1U] & 0x3FU) << 6U) |
                     (unsigned int)((unsigned char)buf[pos + 2U] & 0x3FU);

        if (code_point >= 0x4E00U && code_point <= 0x9FFFU)
            return 2U;
        if (code_point >= 0x3040U && code_point <= 0x309FU)
            return 2U;
        if (code_point >= 0x30A0U && code_point <= 0x30FFU)
            return 2U;
        if (code_point >= 0x3400U && code_point <= 0x4DBFU)
            return 2U;
        if (code_point >= 0x20000U && code_point <= 0x2A6DFU)
            return 2U;
        return 1U;
    }

    if ((first_byte & 0xF8U) == 0xF0U && pos + 3U < len) {
        code_point = ((unsigned int)(first_byte & 0x07U) << 18U) |
                     ((unsigned int)((unsigned char)buf[pos + 1U] & 0x3FU) << 12U) |
                     ((unsigned int)((unsigned char)buf[pos + 2U] & 0x3FU) << 6U) |
                     (unsigned int)((unsigned char)buf[pos + 3U] & 0x3FU);

        if (code_point >= 0x20000U && code_point <= 0x2FFFFU)
            return 2U;
        return 1U;
    }

    return 1U;
}

static size_t pinned_prompt_visible_bytes_from(const char *buf, size_t len,
                                        size_t start, size_t max_cols)
{
    size_t pos;
    size_t cols;
    size_t char_width;

    pos = start;
    cols = 0U;
    while (pos < len && cols < max_cols)
    {
        char_width = utf8_char_display_width(buf, len, pos);
        if (cols + char_width > max_cols)
            break;
        cols += char_width;
        pos = utf8_next_boundary(buf, len, pos);
    }
    return pos - start;
}

static size_t pinned_prompt_display_width_between(const char *buf, size_t len,
                                            size_t start, size_t end)
{
    size_t pos;
    size_t width;

    if (end > len)
        end = len;
    
    pos = start;
    width = 0U;
    while (pos < end)
    {
        width += utf8_char_display_width(buf, len, pos);
        pos = utf8_next_boundary(buf, len, pos);
    }
    return width;
}


static FILE *pinned_prompt_channel_file(com_util_pinned_prompt_channel_t channel)
{
    return channel == COM_UTIL_PINNED_PROMPT_STDERR ? stderr : stdout;
}

static void pinned_prompt_lock(com_util_pinned_prompt_t *screen)
{
    if (screen != NULL && screen->mutex_active)
    {
        (void)com_util_mutex_lock(&screen->mutex);
    }
}

static void pinned_prompt_unlock(com_util_pinned_prompt_t *screen)
{
    if (screen != NULL && screen->mutex_active)
    {
        (void)com_util_mutex_unlock(&screen->mutex);
    }
}

#if defined(PLATFORM_LINUX)

static int pinned_prompt_platform_is_tty(void)
{
    return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
}

static void pinned_prompt_platform_get_size(int *cols, int *rows)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0U && ws.ws_row > 0U)
    {
        *cols = (int)ws.ws_col;
        *rows = (int)ws.ws_row;
        return;
    }
    *cols = 80;
    *rows = 24;
}

static void pinned_prompt_platform_enter_raw(com_util_pinned_prompt_t *screen)
{
    struct termios raw;

    if (screen->raw_active)
    {
        return;
    }
    if (tcgetattr(STDIN_FILENO, &screen->orig_term) != 0)
    {
        return;
    }
    raw = screen->orig_term;
    raw.c_lflag &= ~((tcflag_t)(ICANON | ECHO | ISIG));
    raw.c_iflag &= ~((tcflag_t)(ICRNL | IXON));
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0)
    {
        screen->raw_active = 1;
    }
}

static void pinned_prompt_platform_leave_raw(com_util_pinned_prompt_t *screen)
{
    if (!screen->raw_active)
    {
        return;
    }
    (void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &screen->orig_term);
    screen->raw_active = 0;
}

static int pinned_prompt_platform_read_char(com_util_pinned_prompt_t *screen)
{
    unsigned char c;
    ssize_t       n;

    (void)screen;
    do
    {
        n = read(STDIN_FILENO, &c, 1);
    } while (n < 0 && errno == EINTR);

    return n == 1 ? (int)c : -1;
}

static int pinned_prompt_platform_read_char_nb(com_util_pinned_prompt_t *screen)
{
    fd_set         fds;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    tv.tv_sec = 0;
    tv.tv_usec = 50000;

    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) <= 0)
    {
        return -1;
    }
    return pinned_prompt_platform_read_char(screen);
}

#elif defined(PLATFORM_WINDOWS)

static int pinned_prompt_platform_is_tty(void)
{
    HANDLE in_handle;
    HANDLE out_handle;
    DWORD  mode;

    in_handle = GetStdHandle(STD_INPUT_HANDLE);
    out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (in_handle == INVALID_HANDLE_VALUE || out_handle == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    return GetConsoleMode(in_handle, &mode) && GetConsoleMode(out_handle, &mode);
}

static void pinned_prompt_platform_get_size(int *cols, int *rows)
{
    HANDLE                     out_handle;
    CONSOLE_SCREEN_BUFFER_INFO info;

    out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out_handle != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(out_handle, &info))
    {
        *cols = (int)(info.srWindow.Right - info.srWindow.Left + 1);
        *rows = (int)(info.srWindow.Bottom - info.srWindow.Top + 1);
        return;
    }
    *cols = 80;
    *rows = 24;
}

static void pinned_prompt_platform_enter_raw(com_util_pinned_prompt_t *screen)
{
    DWORD new_mode;

    if (screen->raw_active)
    {
        return;
    }
    screen->stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    if (screen->stdin_handle == INVALID_HANDLE_VALUE)
    {
        return;
    }
    if (!GetConsoleMode(screen->stdin_handle, &screen->orig_in_mode))
    {
        return;
    }
    new_mode = (screen->orig_in_mode | ENABLE_VIRTUAL_TERMINAL_INPUT)
               & ~((DWORD)(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT));
    if (SetConsoleMode(screen->stdin_handle, new_mode))
    {
        screen->raw_active = 1;
    }
}

static void pinned_prompt_platform_leave_raw(com_util_pinned_prompt_t *screen)
{
    if (!screen->raw_active)
    {
        return;
    }
    (void)SetConsoleMode(screen->stdin_handle, screen->orig_in_mode);
    screen->raw_active = 0;
}

static int pinned_prompt_platform_read_char(com_util_pinned_prompt_t *screen)
{
    DWORD n_read;
    char  ch;

    if (!ReadFile(screen->stdin_handle, &ch, 1, &n_read, NULL) || n_read == 0U)
    {
        return -1;
    }
    return (unsigned char)ch;
}

static int pinned_prompt_platform_read_char_nb(com_util_pinned_prompt_t *screen)
{
    DWORD result;

    result = WaitForSingleObject(screen->stdin_handle, 50);
    if (result != WAIT_OBJECT_0)
    {
        return -1;
    }
    return pinned_prompt_platform_read_char(screen);
}

#else

static int pinned_prompt_platform_is_tty(void)
{
    return 0;
}

static void pinned_prompt_platform_get_size(int *cols, int *rows)
{
    *cols = 80;
    *rows = 24;
}

static void pinned_prompt_platform_enter_raw(com_util_pinned_prompt_t *screen)
{
    (void)screen;
}

static void pinned_prompt_platform_leave_raw(com_util_pinned_prompt_t *screen)
{
    (void)screen;
}

static int pinned_prompt_platform_read_char(com_util_pinned_prompt_t *screen)
{
    (void)screen;
    return -1;
}

static int pinned_prompt_platform_read_char_nb(com_util_pinned_prompt_t *screen)
{
    (void)screen;
    return -1;
}

#endif

static void pinned_prompt_update_size(com_util_pinned_prompt_t *screen)
{
    int cols;
    int rows;

    pinned_prompt_platform_get_size(&cols, &rows);
    screen->cols = cols > 0 ? cols : 80;
    screen->rows = rows > 0 ? rows : 24;
}

static int pinned_prompt_set_prompt(com_util_pinned_prompt_t *screen, const char *prompt_str)
{
    size_t len;
    char  *new_buf;

    len = cstr_len(prompt_str);
    if (len + 1U > screen->prompt_cap)
    {
        new_buf = (char *)realloc(screen->prompt_buf, len + 1U);
        if (new_buf == NULL)
        {
            return -1;
        }
        screen->prompt_buf = new_buf;
        screen->prompt_cap = len + 1U;
    }
    if (prompt_str != NULL)
    {
        (void)com_util_strcpy(screen->prompt_buf, screen->prompt_cap, prompt_str);
    }
    else
    {
        screen->prompt_buf[0] = '\0';
    }
    return 0;
}

static void pinned_prompt_adjust_view(com_util_pinned_prompt_t *screen)
{
    size_t prompt_cols;
    size_t input_cols;
    size_t cursor_cols;

    prompt_cols = cstr_len(screen->prompt_buf);
    if ((size_t)screen->cols > prompt_cols + 1U)
    {
        input_cols = (size_t)screen->cols - prompt_cols - 1U;
    }
    else
    {
        input_cols = 1U;
    }

    screen->view_start = utf8_sanitize_boundary(screen->edit_buf,
                                                screen->edit_len,
                                                screen->view_start);
    if (screen->cursor < screen->view_start)
    {
        screen->view_start = screen->cursor;
    }
    cursor_cols = pinned_prompt_display_width_between(screen->edit_buf,
                                             screen->edit_len,
                                             screen->view_start,
                                             screen->cursor);
    while (cursor_cols > input_cols)
    {
        screen->view_start = utf8_next_boundary(screen->edit_buf,
                                                screen->edit_len,
                                                screen->view_start);
        cursor_cols = pinned_prompt_display_width_between(screen->edit_buf,
                                                 screen->edit_len,
                                                 screen->view_start,
                                                 screen->cursor);
    }
}

static void pinned_prompt_render_separator(com_util_pinned_prompt_t *screen, int row)
{
    int i;
    int separator_width;

    (void)printf("\033[%d;1H\033[2K", row);
    separator_width = screen->cols - 1;
    for (i = 0; i < separator_width; i++)
    {
        (void)fputc('-', stdout);
    }
}

static void pinned_prompt_render_status_line(com_util_pinned_prompt_t *screen,
                                             int                        row,
                                             const char                *left_content,
                                             const char                *right_content)
{
    size_t left_len;
    size_t right_len;
    size_t left_width;
    size_t right_width;
    size_t middle_spaces;
    int    i;
    int    line_width;

    left_len = cstr_len(left_content);
    right_len = cstr_len(right_content);

    left_width = pinned_prompt_display_width_between(left_content, left_len, 0U, left_len);
    right_width = pinned_prompt_display_width_between(right_content, right_len, 0U, right_len);

    (void)printf("\033[%d;1H\033[2K", row);

    line_width = screen->cols - 1;

    if (left_content != NULL && left_len > 0U)
    {
        (void)fputs(left_content, stdout);
    }

    if (left_width + right_width < (size_t)line_width)
    {
        middle_spaces = (size_t)line_width - left_width - right_width;
        for (i = 0; i < (int)middle_spaces; i++)
        {
            (void)fputc(' ', stdout);
        }
    }

    if (right_content != NULL && right_len > 0U)
    {
        (void)fputs(right_content, stdout);
    }
}

static void pinned_prompt_calc_layout(com_util_pinned_prompt_t *screen,
                                      pinned_prompt_layout_t   *layout)
{
    int rows;
    int first_control_row;

    rows = screen->rows > 0 ? screen->rows : 1;

    layout->show_bottom_status = screen->status_bottom_enabled && rows >= 3;
    layout->prompt_row = layout->show_bottom_status ? rows - 2 : rows;
    layout->prompt_sep_row = layout->prompt_row - 1;

    layout->show_top_status = screen->status_top_enabled && layout->prompt_row >= 4;
    if (layout->show_top_status)
    {
        layout->top_status_row = layout->prompt_sep_row - 1;
        first_control_row = layout->top_status_row;
    }
    else
    {
        layout->top_status_row = 0;
        first_control_row = layout->prompt_sep_row;
    }
    layout->main_bottom_row = first_control_row - 1;

    if (layout->show_bottom_status)
    {
        layout->bottom_sep_row = layout->prompt_row + 1;
        layout->bottom_status_row = layout->prompt_row + 2;
    }
    else
    {
        layout->bottom_sep_row = 0;
        layout->bottom_status_row = 0;
    }

    if (layout->prompt_row < 1)
    {
        layout->prompt_row = 1;
    }
    if (layout->prompt_sep_row < 1)
    {
        layout->prompt_sep_row = 1;
    }
    if (layout->main_bottom_row < 1)
    {
        layout->main_bottom_row = 1;
    }
}

static void pinned_prompt_clear_control_area(com_util_pinned_prompt_t       *screen,
                                             const pinned_prompt_layout_t   *layout)
{
    int row;

    for (row = layout->main_bottom_row + 1; row <= screen->rows; row++)
    {
        (void)printf("\033[%d;1H\033[2K", row);
    }
}

static void pinned_prompt_render_locked(com_util_pinned_prompt_t *screen)
{
    size_t prompt_cols;
    size_t input_cols;
    size_t visible_bytes;
    size_t cursor_cols;
    size_t cursor_col;
    pinned_prompt_layout_t layout;

    if (!screen->is_tty || !screen->prompt_visible)
    {
        return;
    }

    pinned_prompt_update_size(screen);
    pinned_prompt_adjust_view(screen);
    pinned_prompt_calc_layout(screen, &layout);

    prompt_cols = cstr_len(screen->prompt_buf);
    input_cols = (size_t)screen->cols > prompt_cols + 1U
               ? (size_t)screen->cols - prompt_cols - 1U
               : 1U;
    visible_bytes = pinned_prompt_visible_bytes_from(screen->edit_buf,
                                              screen->edit_len,
                                              screen->view_start,
                                              input_cols);
    cursor_cols = pinned_prompt_display_width_between(screen->edit_buf,
                                             screen->edit_len,
                                             screen->view_start,
                                             screen->cursor);
    cursor_col = prompt_cols + cursor_cols + 1U;
    if (cursor_col > (size_t)screen->cols)
    {
        cursor_col = (size_t)screen->cols;
    }

    (void)printf("\033[?25l");

    if (screen->status_dirty)
    {
        pinned_prompt_clear_control_area(screen, &layout);

        if ((!screen->status_top_enabled || layout.show_top_status) &&
            (!screen->status_bottom_enabled || layout.show_bottom_status))
        {
            screen->status_dirty = 0;
        }
    }

    if (layout.show_top_status)
    {
        pinned_prompt_render_status_line(screen, layout.top_status_row,
                                         screen->status_top_left,
                                         screen->status_top_right);
    }
    pinned_prompt_render_separator(screen, layout.prompt_sep_row);

    if (layout.show_bottom_status)
    {
        pinned_prompt_render_separator(screen, layout.bottom_sep_row);
        pinned_prompt_render_status_line(screen, layout.bottom_status_row,
                                         screen->status_bottom_left,
                                         screen->status_bottom_right);
    }

    (void)printf("\033[%d;1H\033[2K", layout.prompt_row);
    (void)fputs(screen->prompt_buf != NULL ? screen->prompt_buf : "", stdout);
    if (visible_bytes > 0U)
    {
        (void)fwrite(screen->edit_buf + screen->view_start, 1U, visible_bytes, stdout);
    }

    (void)printf("\033[%d;%zuH\033[?25h", layout.prompt_row, cursor_col);
    (void)fflush(stdout);
}

static void pinned_prompt_hide_prompt_locked(com_util_pinned_prompt_t *screen)
{
    pinned_prompt_layout_t layout;

    if (!screen->is_tty || !screen->prompt_visible)
    {
        return;
    }
    pinned_prompt_update_size(screen);
    pinned_prompt_calc_layout(screen, &layout);
    (void)printf("\033[%d;1H\033[2K", layout.prompt_row);
    (void)fflush(stdout);
}

static void pinned_prompt_finish_prompt_locked(com_util_pinned_prompt_t *screen)
{
    if (!screen->is_tty || !screen->prompt_visible)
    {
        return;
    }
    pinned_prompt_hide_prompt_locked(screen);
    screen->prompt_visible = 0;
}

static void pinned_prompt_cleanup_terminal_locked(com_util_pinned_prompt_t *screen)
{
    pinned_prompt_layout_t layout;

    if (!screen->is_tty)
    {
        return;
    }

    pinned_prompt_update_size(screen);
    pinned_prompt_calc_layout(screen, &layout);
    (void)printf("\033[r");
    pinned_prompt_clear_control_area(screen, &layout);
    (void)printf("\033[%d;1H\033[?25h", layout.prompt_row);
    (void)fflush(stdout);
    screen->prompt_visible = 0;
}

static void pinned_prompt_prepare_output_locked(com_util_pinned_prompt_t *screen)
{
    pinned_prompt_layout_t layout;

    if (!screen->is_tty)
    {
        return;
    }
    pinned_prompt_update_size(screen);
    pinned_prompt_calc_layout(screen, &layout);
    if (screen->prompt_visible)
    {
        pinned_prompt_hide_prompt_locked(screen);
    }

    if (layout.main_bottom_row < screen->rows)
    {
        (void)printf("\033[1;%dr", layout.main_bottom_row);
    }
    (void)printf("\033[%d;1H\033[2K", layout.main_bottom_row);
    (void)fflush(stdout);
}

static pinned_prompt_key_t pinned_prompt_read_key(com_util_pinned_prompt_t *screen, int *out_ch)
{
    int c;
    int c2;
    int c3;
    int c4;

    c = pinned_prompt_platform_read_char(screen);
    if (c == -1)
    {
        return PINNED_PROMPT_KEY_EOF;
    }
    if (c == '\r' || c == '\n')
    {
        return PINNED_PROMPT_KEY_ENTER;
    }
    if (c == 0x03)
    {
        return PINNED_PROMPT_KEY_CTRL_C;
    }
    if (c == 0x7F || c == 0x08)
    {
        return PINNED_PROMPT_KEY_BACKSPACE;
    }
    if (c == 0x1B)
    {
        c2 = pinned_prompt_platform_read_char_nb(screen);
        if (c2 == '[')
        {
            c3 = pinned_prompt_platform_read_char_nb(screen);
            switch (c3)
            {
            case 'A': return PINNED_PROMPT_KEY_UP;
            case 'B': return PINNED_PROMPT_KEY_DOWN;
            case 'C': return PINNED_PROMPT_KEY_RIGHT;
            case 'D': return PINNED_PROMPT_KEY_LEFT;
            case 'H': return PINNED_PROMPT_KEY_HOME;
            case 'F': return PINNED_PROMPT_KEY_END;
            case '1':
                c4 = pinned_prompt_platform_read_char_nb(screen);
                return c4 == '~' ? PINNED_PROMPT_KEY_HOME : PINNED_PROMPT_KEY_UNKNOWN;
            case '3':
                c4 = pinned_prompt_platform_read_char_nb(screen);
                return c4 == '~' ? PINNED_PROMPT_KEY_DELETE : PINNED_PROMPT_KEY_UNKNOWN;
            case '4':
                c4 = pinned_prompt_platform_read_char_nb(screen);
                return c4 == '~' ? PINNED_PROMPT_KEY_END : PINNED_PROMPT_KEY_UNKNOWN;
            default:
                return PINNED_PROMPT_KEY_UNKNOWN;
            }
        }
        return PINNED_PROMPT_KEY_UNKNOWN;
    }
    if (c >= 0x20 || (unsigned char)c >= 0x80U)
    {
        *out_ch = c;
        return PINNED_PROMPT_KEY_CHAR;
    }
    return PINNED_PROMPT_KEY_UNKNOWN;
}

static void pinned_prompt_history_add(com_util_pinned_prompt_t *screen, const char *line)
{
    size_t len;
    size_t slot;

    if (screen->history_max == 0U || line == NULL || line[0] == '\0')
    {
        return;
    }
    if (screen->history_count > 0U)
    {
        slot = PINNED_PROMPT_HIST_IDX(screen, screen->history_count - 1U);
        if (screen->history[slot] != NULL && strcmp(screen->history[slot], line) == 0)
        {
            return;
        }
    }
    if (screen->history_count == screen->history_max)
    {
        free(screen->history[screen->history_head]);
        screen->history[screen->history_head] = NULL;
        screen->history_head = (screen->history_head + 1U) % screen->history_max;
    }
    else
    {
        screen->history_count++;
    }

    slot = PINNED_PROMPT_HIST_IDX(screen, screen->history_count - 1U);
    len = strlen(line) + 1U;
    screen->history[slot] = (char *)malloc(len);
    if (screen->history[slot] != NULL)
    {
        (void)com_util_strcpy(screen->history[slot], len, line);
    }
}

static void pinned_prompt_set_edit_line(com_util_pinned_prompt_t *screen, const char *line)
{
    size_t len;

    len = cstr_len(line);
    if (len >= screen->edit_cap)
    {
        len = screen->edit_cap - 1U;
    }
    if (line != NULL && len > 0U)
    {
        memcpy(screen->edit_buf, line, len);
    }
    screen->edit_buf[len] = '\0';
    screen->edit_len = len;
    screen->cursor = len;
    screen->view_start = 0U;
}

static void pinned_prompt_history_prev(com_util_pinned_prompt_t *screen)
{
    const char *entry;
    size_t      slot;

    if (screen->history_count == 0U)
    {
        return;
    }
    if (screen->browse_idx == -1)
    {
        (void)com_util_strcpy(screen->saved_line, screen->edit_cap, screen->edit_buf);
        screen->browse_idx = (int)screen->history_count - 1;
    }
    else if (screen->browse_idx > 0)
    {
        screen->browse_idx--;
    }
    else
    {
        return;
    }
    slot = PINNED_PROMPT_HIST_IDX(screen, (size_t)screen->browse_idx);
    entry = screen->history[slot];
    if (entry != NULL)
    {
        pinned_prompt_set_edit_line(screen, entry);
    }
}

static void pinned_prompt_history_next(com_util_pinned_prompt_t *screen)
{
    const char *entry;
    size_t      slot;

    if (screen->browse_idx == -1)
    {
        return;
    }
    if (screen->browse_idx < (int)screen->history_count - 1)
    {
        screen->browse_idx++;
        slot = PINNED_PROMPT_HIST_IDX(screen, (size_t)screen->browse_idx);
        entry = screen->history[slot];
        if (entry != NULL)
        {
            pinned_prompt_set_edit_line(screen, entry);
        }
    }
    else
    {
        pinned_prompt_set_edit_line(screen, screen->saved_line);
        screen->browse_idx = -1;
    }
}

static void pinned_prompt_insert_byte(com_util_pinned_prompt_t *screen, int ch)
{
    if (screen->edit_len + 1U >= screen->edit_cap)
    {
        return;
    }
    memmove(screen->edit_buf + screen->cursor + 1U,
            screen->edit_buf + screen->cursor,
            screen->edit_len - screen->cursor + 1U);
    screen->edit_buf[screen->cursor] = (char)ch;
    screen->cursor++;
    screen->edit_len++;
}

static void pinned_prompt_backspace(com_util_pinned_prompt_t *screen)
{
    size_t prev;

    if (screen->cursor == 0U)
    {
        return;
    }
    prev = utf8_prev_boundary(screen->edit_buf, screen->cursor);
    memmove(screen->edit_buf + prev,
            screen->edit_buf + screen->cursor,
            screen->edit_len - screen->cursor + 1U);
    screen->edit_len -= screen->cursor - prev;
    screen->cursor = prev;
}

static void pinned_prompt_delete(com_util_pinned_prompt_t *screen)
{
    size_t next;

    if (screen->cursor >= screen->edit_len)
    {
        return;
    }
    next = utf8_next_boundary(screen->edit_buf, screen->edit_len, screen->cursor);
    memmove(screen->edit_buf + screen->cursor,
            screen->edit_buf + next,
            screen->edit_len - next + 1U);
    screen->edit_len -= next - screen->cursor;
}

static int pinned_prompt_format_prompt(com_util_pinned_prompt_t *screen, const char *fmt, va_list ap)
{
    va_list ap_copy;
    int     needed;
    char   *new_buf;

    if (screen->fmt_buf == NULL)
    {
        screen->fmt_cap = 256U;
        screen->fmt_buf = (char *)malloc(screen->fmt_cap);
        if (screen->fmt_buf == NULL)
        {
            return -1;
        }
    }

    for (;;)
    {
        va_copy(ap_copy, ap);
        needed = vsnprintf(screen->fmt_buf, screen->fmt_cap, fmt != NULL ? fmt : "", ap_copy);
        va_end(ap_copy);

        if (needed < 0)
        {
            screen->fmt_buf[0] = '\0';
            return 0;
        }
        if ((size_t)needed < screen->fmt_cap)
        {
            return 0;
        }
        new_buf = (char *)realloc(screen->fmt_buf, (size_t)needed + 1U);
        if (new_buf == NULL)
        {
            screen->fmt_buf[screen->fmt_cap - 1U] = '\0';
            return 0;
        }
        screen->fmt_buf = new_buf;
        screen->fmt_cap = (size_t)needed + 1U;
    }
}

static int pinned_prompt_readline_fallback(char *buf, size_t buf_size, const char *prompt_str)
{
    if (prompt_str != NULL)
    {
        (void)fputs(prompt_str, stdout);
        (void)fflush(stdout);
    }
    if (fgets(buf, (int)buf_size, stdin) == NULL)
    {
        return 0;
    }
    buf[strcspn(buf, "\r\n")] = '\0';
    return 1;
}

com_util_pinned_prompt_t *com_util_pinned_prompt_create(const com_util_pinned_prompt_options_t *options)
{
    com_util_pinned_prompt_t *screen;
    size_t                    history_max;

    com_util_console_init();

    screen = (com_util_pinned_prompt_t *)calloc(1U, sizeof(*screen));
    if (screen == NULL)
    {
        return NULL;
    }

    history_max = options != NULL ? options->history_max : 0U;
    if (history_max == 0U)
    {
        history_max = COM_UTIL_PROMPT_HISTORY_DEFAULT;
    }
    screen->history_max = history_max;
    screen->edit_cap = COM_UTIL_PROMPT_LINE_MAX;
    screen->browse_idx = -1;
    screen->is_tty = pinned_prompt_platform_is_tty();
    pinned_prompt_update_size(screen);

    if (com_util_mutex_init(&screen->mutex) != 0)
    {
        free(screen);
        return NULL;
    }
    screen->mutex_active = 1;

    screen->edit_buf = (char *)malloc(screen->edit_cap);
    screen->saved_line = (char *)malloc(screen->edit_cap);
    screen->history = (char **)calloc(screen->history_max, sizeof(char *));
    screen->prompt_buf = (char *)malloc(1U);
    screen->status_top_left = (char *)malloc(1U);
    screen->status_top_right = (char *)malloc(1U);
    screen->status_bottom_left = (char *)malloc(1U);
    screen->status_bottom_right = (char *)malloc(1U);
    if (screen->edit_buf == NULL || screen->saved_line == NULL ||
        screen->history == NULL || screen->prompt_buf == NULL ||
        screen->status_top_left == NULL || screen->status_top_right == NULL ||
        screen->status_bottom_left == NULL || screen->status_bottom_right == NULL)
    {
        com_util_pinned_prompt_dispose(screen);
        return NULL;
    }
    screen->edit_buf[0] = '\0';
    screen->saved_line[0] = '\0';
    screen->prompt_buf[0] = '\0';
    screen->prompt_cap = 1U;
    screen->status_top_left[0] = '\0';
    screen->status_top_left_cap = 1U;
    screen->status_top_right[0] = '\0';
    screen->status_top_right_cap = 1U;
    screen->status_bottom_left[0] = '\0';
    screen->status_bottom_left_cap = 1U;
    screen->status_bottom_right[0] = '\0';
    screen->status_bottom_right_cap = 1U;
    screen->status_dirty = 1;

    return screen;
}

void com_util_pinned_prompt_dispose(com_util_pinned_prompt_t *screen)
{
    size_t i;

    if (screen == NULL)
    {
        return;
    }
    pinned_prompt_lock(screen);
    pinned_prompt_cleanup_terminal_locked(screen);
    pinned_prompt_platform_leave_raw(screen);
    pinned_prompt_unlock(screen);

    if (screen->history != NULL)
    {
        for (i = 0U; i < screen->history_max; i++)
        {
            free(screen->history[i]);
        }
    }
    free(screen->history);
    free(screen->edit_buf);
    free(screen->saved_line);
    free(screen->prompt_buf);
    free(screen->fmt_buf);
    free(screen->status_top_left);
    free(screen->status_top_right);
    free(screen->status_bottom_left);
    free(screen->status_bottom_right);
    if (screen->mutex_active)
    {
        (void)com_util_mutex_destroy(&screen->mutex);
    }
    free(screen);
}

int com_util_pinned_prompt_readline(com_util_pinned_prompt_t *screen,
                                    char                     *buf,
                                    size_t                    buf_size,
                                    const char               *prompt_str)
{
    int done;
    int result;

    if (screen == NULL || buf == NULL || buf_size == 0U)
    {
        return 0;
    }
    buf[0] = '\0';

    if (!screen->is_tty)
    {
        return pinned_prompt_readline_fallback(buf, buf_size, prompt_str);
    }

    pinned_prompt_platform_enter_raw(screen);
    if (!screen->raw_active)
    {
        return pinned_prompt_readline_fallback(buf, buf_size, prompt_str);
    }

    pinned_prompt_lock(screen);
    if (pinned_prompt_set_prompt(screen, prompt_str) != 0)
    {
        pinned_prompt_unlock(screen);
        pinned_prompt_platform_leave_raw(screen);
        return 0;
    }
    screen->edit_len = 0U;
    screen->edit_buf[0] = '\0';
    screen->cursor = 0U;
    screen->view_start = 0U;
    screen->browse_idx = -1;
    screen->prompt_visible = 1;
    pinned_prompt_render_locked(screen);
    pinned_prompt_unlock(screen);

    done = 0;
    result = 0;
    while (!done)
    {
        int          ch;
        pinned_prompt_key_t key;

        ch = 0;
        key = pinned_prompt_read_key(screen, &ch);

        pinned_prompt_lock(screen);
        switch (key)
        {
        case PINNED_PROMPT_KEY_ENTER:
        {
            size_t copy;

            copy = screen->edit_len < buf_size - 1U ? screen->edit_len : buf_size - 1U;
            if (copy > 0U)
            {
                memcpy(buf, screen->edit_buf, copy);
            }
            buf[copy] = '\0';
            pinned_prompt_history_add(screen, screen->edit_buf);
            pinned_prompt_finish_prompt_locked(screen);
            result = 1;
            done = 1;
            break;
        }
        case PINNED_PROMPT_KEY_EOF:
            buf[0] = '\0';
            pinned_prompt_finish_prompt_locked(screen);
            result = 0;
            done = 1;
            break;
        case PINNED_PROMPT_KEY_CTRL_C:
            buf[0] = '\0';
            pinned_prompt_cleanup_terminal_locked(screen);
            result = 0;
            done = 1;
            break;
        case PINNED_PROMPT_KEY_BACKSPACE:
            pinned_prompt_backspace(screen);
            pinned_prompt_render_locked(screen);
            break;
        case PINNED_PROMPT_KEY_DELETE:
            pinned_prompt_delete(screen);
            pinned_prompt_render_locked(screen);
            break;
        case PINNED_PROMPT_KEY_LEFT:
            screen->cursor = utf8_prev_boundary(screen->edit_buf, screen->cursor);
            pinned_prompt_render_locked(screen);
            break;
        case PINNED_PROMPT_KEY_RIGHT:
            screen->cursor = utf8_next_boundary(screen->edit_buf, screen->edit_len, screen->cursor);
            pinned_prompt_render_locked(screen);
            break;
        case PINNED_PROMPT_KEY_HOME:
            screen->cursor = 0U;
            pinned_prompt_render_locked(screen);
            break;
        case PINNED_PROMPT_KEY_END:
            screen->cursor = screen->edit_len;
            pinned_prompt_render_locked(screen);
            break;
        case PINNED_PROMPT_KEY_UP:
            pinned_prompt_history_prev(screen);
            pinned_prompt_render_locked(screen);
            break;
        case PINNED_PROMPT_KEY_DOWN:
            pinned_prompt_history_next(screen);
            pinned_prompt_render_locked(screen);
            break;
        case PINNED_PROMPT_KEY_CHAR:
            pinned_prompt_insert_byte(screen, ch);
            pinned_prompt_render_locked(screen);
            break;
        case PINNED_PROMPT_KEY_UNKNOWN:
        default:
            pinned_prompt_render_locked(screen);
            break;
        }
        pinned_prompt_unlock(screen);
    }

    pinned_prompt_platform_leave_raw(screen);
    return result;
}

int com_util_pinned_prompt_readline_fmt(com_util_pinned_prompt_t *screen,
                                        char                     *buf,
                                        size_t                    buf_size,
                                        const char               *fmt,
                                        ...)
{
    va_list ap;
    int     rc;

    if (screen == NULL)
    {
        return 0;
    }
    va_start(ap, fmt);
    rc = pinned_prompt_format_prompt(screen, fmt, ap);
    va_end(ap);
    if (rc != 0)
    {
        return com_util_pinned_prompt_readline(screen, buf, buf_size, "");
    }
    return com_util_pinned_prompt_readline(screen, buf, buf_size, screen->fmt_buf);
}

size_t com_util_pinned_prompt_write(com_util_pinned_prompt_t         *screen,
                                    com_util_pinned_prompt_channel_t  channel,
                                    const void                       *data,
                                    size_t                            size)
{
    FILE  *out;
    size_t written;
    int    append_newline;

    if (screen == NULL || (data == NULL && size != 0U))
    {
        return 0U;
    }

    out = pinned_prompt_channel_file(channel);
    if (!screen->is_tty)
    {
        written = size > 0U ? fwrite(data, 1U, size, out) : 0U;
        (void)fflush(out);
        return written;
    }

    pinned_prompt_lock(screen);
    append_newline = size > 0U && ((const char *)data)[size - 1U] != '\n';
    pinned_prompt_prepare_output_locked(screen);
    if (size > 0U)
    {
        written = fwrite(data, 1U, size, out);
    }
    else
    {
        written = 0U;
    }
    if (append_newline)
    {
        (void)fputc('\n', out);
    }
    (void)fflush(out);
    (void)printf("\033[r");
    pinned_prompt_render_locked(screen);
    pinned_prompt_unlock(screen);

    return written;
}

int com_util_pinned_prompt_printf(com_util_pinned_prompt_t         *screen,
                                  com_util_pinned_prompt_channel_t  channel,
                                  const char                       *fmt,
                                  ...)
{
    va_list ap;
    va_list ap_copy;
    int     needed;
    char   *buf;
    size_t  written;

    if (screen == NULL)
    {
        return -1;
    }

    va_start(ap, fmt);
    va_copy(ap_copy, ap);
    needed = vsnprintf(NULL, 0U, fmt != NULL ? fmt : "", ap_copy);
    va_end(ap_copy);
    if (needed < 0)
    {
        va_end(ap);
        return -1;
    }

    buf = (char *)malloc((size_t)needed + 1U);
    if (buf == NULL)
    {
        va_end(ap);
        return -1;
    }
    (void)vsnprintf(buf, (size_t)needed + 1U, fmt != NULL ? fmt : "", ap);
    va_end(ap);

    written = com_util_pinned_prompt_write(screen, channel, buf, (size_t)needed);
    free(buf);
    return (int)written;
}

int com_util_pinned_prompt_status_enable(com_util_pinned_prompt_t                 *screen,
                                         com_util_pinned_prompt_status_position_t  position,
                                         int                                       enable)
{
    if (screen == NULL)
    {
        return -1;
    }

    pinned_prompt_lock(screen);
    if (position == COM_UTIL_PINNED_PROMPT_STATUS_TOP)
    {
        screen->status_top_enabled = enable ? 1 : 0;
    }
    else if (position == COM_UTIL_PINNED_PROMPT_STATUS_BOTTOM)
    {
        screen->status_bottom_enabled = enable ? 1 : 0;
    }
    else
    {
        pinned_prompt_unlock(screen);
        return -1;
    }
    screen->status_dirty = 1;
    pinned_prompt_render_locked(screen);
    pinned_prompt_unlock(screen);
    return 0;
}

static int pinned_prompt_set_status_content(char **buf, size_t *cap, const char *content)
{
    size_t len;
    char  *new_buf;

    len = cstr_len(content);
    if (len + 1U > *cap)
    {
        new_buf = (char *)realloc(*buf, len + 1U);
        if (new_buf == NULL)
        {
            return -1;
        }
        *buf = new_buf;
        *cap = len + 1U;
    }
    if (content != NULL)
    {
        (void)com_util_strcpy(*buf, *cap, content);
    }
    else
    {
        (*buf)[0] = '\0';
    }
    return 0;
}

int com_util_pinned_prompt_status_set(com_util_pinned_prompt_t                 *screen,
                                      com_util_pinned_prompt_status_position_t  position,
                                      com_util_pinned_prompt_status_align_t     align,
                                      const char                               *content)
{
    int rc;

    if (screen == NULL)
    {
        return -1;
    }

    pinned_prompt_lock(screen);
    if (position == COM_UTIL_PINNED_PROMPT_STATUS_TOP)
    {
        if (align == COM_UTIL_PINNED_PROMPT_STATUS_LEFT)
        {
            rc = pinned_prompt_set_status_content(&screen->status_top_left,
                                                  &screen->status_top_left_cap,
                                                  content);
        }
        else if (align == COM_UTIL_PINNED_PROMPT_STATUS_RIGHT)
        {
            rc = pinned_prompt_set_status_content(&screen->status_top_right,
                                                  &screen->status_top_right_cap,
                                                  content);
        }
        else
        {
            pinned_prompt_unlock(screen);
            return -1;
        }
    }
    else if (position == COM_UTIL_PINNED_PROMPT_STATUS_BOTTOM)
    {
        if (align == COM_UTIL_PINNED_PROMPT_STATUS_LEFT)
        {
            rc = pinned_prompt_set_status_content(&screen->status_bottom_left,
                                                  &screen->status_bottom_left_cap,
                                                  content);
        }
        else if (align == COM_UTIL_PINNED_PROMPT_STATUS_RIGHT)
        {
            rc = pinned_prompt_set_status_content(&screen->status_bottom_right,
                                                  &screen->status_bottom_right_cap,
                                                  content);
        }
        else
        {
            pinned_prompt_unlock(screen);
            return -1;
        }
    }
    else
    {
        pinned_prompt_unlock(screen);
        return -1;
    }

    if (rc == 0)
    {
        screen->status_dirty = 1;
        pinned_prompt_render_locked(screen);
    }
    pinned_prompt_unlock(screen);
    return rc;
}
