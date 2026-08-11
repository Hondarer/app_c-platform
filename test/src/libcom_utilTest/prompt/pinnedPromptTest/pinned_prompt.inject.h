#ifndef PINNED_PROMPT_INJECT_H
#define PINNED_PROMPT_INJECT_H

#include <stddef.h>

#include <com_util/prompt/pinned_prompt.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /* pinned_prompt.c の static 関数 cstr_len へのアクセサー。 */
    extern size_t test_pinned_prompt_cstr_len(const char *text);

    /* UTF-8 の表示幅判定へ直接アクセスするアクセサー。 */
    extern size_t test_pinned_prompt_utf8_width(const char *text, size_t len, size_t pos);

    /* ANSI SGR シーケンス長判定へ直接アクセスするアクセサー。 */
    extern size_t test_pinned_prompt_ansi_len(const char *text, size_t len, size_t pos);

    /* 表示列数に基づく可視バイト数計算へ直接アクセスするアクセサー。 */
    extern size_t test_pinned_prompt_visible_bytes(const char *text, size_t len, size_t start, size_t max_cols);

    /* 指定範囲の表示幅計算へ直接アクセスするアクセサー。 */
    extern size_t test_pinned_prompt_display_width(const char *text, size_t len, size_t start, size_t end);

    /* 非 TTY のフォールバック経路を選択するためのテスト用状態変更。 */
    extern void test_pinned_prompt_set_tty(com_util_pinned_prompt *screen, int is_tty);

    /* 内部キー分類のテスト用値です。 */
    enum test_pinned_prompt_key
    {
        TEST_PINNED_PROMPT_KEY_CHAR = 0,
        TEST_PINNED_PROMPT_KEY_ENTER,
        TEST_PINNED_PROMPT_KEY_BACKSPACE,
        TEST_PINNED_PROMPT_KEY_DELETE,
        TEST_PINNED_PROMPT_KEY_LEFT,
        TEST_PINNED_PROMPT_KEY_RIGHT,
        TEST_PINNED_PROMPT_KEY_UP,
        TEST_PINNED_PROMPT_KEY_DOWN,
        TEST_PINNED_PROMPT_KEY_HOME,
        TEST_PINNED_PROMPT_KEY_END,
        TEST_PINNED_PROMPT_KEY_CTRL_C,
        TEST_PINNED_PROMPT_KEY_CLEAR,
        TEST_PINNED_PROMPT_KEY_RESIZE,
        TEST_PINNED_PROMPT_KEY_UNKNOWN,
        TEST_PINNED_PROMPT_KEY_EOF
    };

    /* 端末入力のキー分類へ直接アクセスする。 */
    extern int test_pinned_prompt_read_key(com_util_pinned_prompt *screen, int *out_ch);

    /* 行編集 static 関数を個別に検証する。 */
    extern void test_pinned_prompt_set_edit_line(com_util_pinned_prompt *screen, const char *line);
    extern void test_pinned_prompt_set_cursor(com_util_pinned_prompt *screen, size_t cursor);
    extern void test_pinned_prompt_insert_byte(com_util_pinned_prompt *screen, int ch);
    extern void test_pinned_prompt_backspace(com_util_pinned_prompt *screen);
    extern void test_pinned_prompt_delete(com_util_pinned_prompt *screen);
    extern const char *test_pinned_prompt_edit_text(const com_util_pinned_prompt *screen);
    extern size_t test_pinned_prompt_edit_length(const com_util_pinned_prompt *screen);

    /* TTY 描画 static 関数を検証するための状態設定と描画。 */
    extern void test_pinned_prompt_render_state(com_util_pinned_prompt *screen, int is_tty, int prompt_visible,
                                                int status_top_enabled, int status_bottom_enabled,
                                                const char *prompt, const char *edit_line, const char *top_left,
                                                const char *top_right, const char *bottom_left,
                                                const char *bottom_right);
    extern void test_pinned_prompt_render(com_util_pinned_prompt *screen);

    /* 履歴と出力準備の境界経路へ直接アクセスする。 */
    extern void test_pinned_prompt_history_edge_cases(com_util_pinned_prompt *screen);
    extern void test_pinned_prompt_prepare_output(com_util_pinned_prompt *screen);

#if defined(PLATFORM_LINUX)
    /* Linux プラットフォーム層の static 状態をテスト開始状態へ戻す。 */
    extern void test_pinned_prompt_reset_platform_state(void);

    /* EINTR とリサイズ通知の経路を指定するための状態変更。 */
    extern void test_pinned_prompt_set_resize_pending(int value);

    /* リサイズ通知状態を取得する。 */
    extern int test_pinned_prompt_resize_pending(void);

    /* raw モード状態を取得する。 */
    extern int test_pinned_prompt_raw_active(const com_util_pinned_prompt *screen);

    /* 端末サイズ取得へ直接アクセスする。 */
    extern void test_pinned_prompt_get_size(int *cols, int *rows);

    /* raw モード移行・復帰へ直接アクセスする。 */
    extern void test_pinned_prompt_enter_raw(com_util_pinned_prompt *screen);
    extern void test_pinned_prompt_leave_raw(com_util_pinned_prompt *screen);

    /* 端末入力の blocking/non-blocking 読み取りへ直接アクセスする。 */
    extern int test_pinned_prompt_read_char(com_util_pinned_prompt *screen);
    extern int test_pinned_prompt_read_char_nb(com_util_pinned_prompt *screen);
#endif /* PLATFORM_LINUX */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PINNED_PROMPT_INJECT_H */
