/**
 *  @file           prompt_windows.c
 *  @brief          プロンプト ヘルパー Windows 実装 (SetConsoleMode VT 入力) です。
 */

#include <com_util/prompt/prompt_internal.h>

#if defined(PLATFORM_WINDOWS)

/* Doxygen コメントは、ヘッダーに記載 */

void prompt_platform_enter_raw(com_util_prompt *p)
{
    DWORD new_mode;
    if (p->raw_active)
    {
        return;
    }
    p->stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    if (p->stdin_handle == INVALID_HANDLE_VALUE)
    {
        return;
    }
    if (!GetConsoleMode(p->stdin_handle, &p->orig_in_mode))
    {
        return;
    }
    /* ENABLE_VIRTUAL_TERMINAL_INPUT : ESC シーケンスを仮想端末入力として受け取る
     * ENABLE_ECHO_INPUT              : エコー無効
     * ENABLE_LINE_INPUT              : 行単位読み取り無効 (文字単位に変更)
     * ENABLE_PROCESSED_INPUT         : Ctrl+C をシグナルではなく文字として受け取る */
    new_mode = (p->orig_in_mode | ENABLE_VIRTUAL_TERMINAL_INPUT) &
               ~((DWORD)(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT));
    if (SetConsoleMode(p->stdin_handle, new_mode))
    {
        p->raw_active = 1;
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

void prompt_platform_leave_raw(com_util_prompt *p)
{
    if (!p->raw_active)
    {
        return;
    }
    SetConsoleMode(p->stdin_handle, p->orig_in_mode);
    p->raw_active = 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

int prompt_platform_read_char(com_util_prompt *p)
{
    DWORD result;
    DWORD n_read;
    char ch;

    result = WaitForSingleObject(p->stdin_handle, 100U);
    if (result == WAIT_TIMEOUT)
    {
        return -2; /* リサイズ チェック用タイムアウト */
    }
    if (result != WAIT_OBJECT_0)
    {
        return -1;
    }
    if (!ReadFile(p->stdin_handle, &ch, 1, &n_read, NULL) || n_read == 0)
    {
        return -1;
    }
    return (unsigned char)ch;
}

/* Doxygen コメントは、ヘッダーに記載 */

int prompt_platform_read_char_nb(com_util_prompt *p)
{
    DWORD result = WaitForSingleObject(p->stdin_handle, 50); /* 50ms */
    if (result != WAIT_OBJECT_0)
    {
        return -1;
    }
    return prompt_platform_read_char(p);
}

#endif /* PLATFORM_WINDOWS */
