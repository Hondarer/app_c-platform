/**
 *  @file           prompt_linux.c
 *  @brief          プロンプトヘルパー Linux 実装（termios raw mode）。
 */

#include <com_util/prompt/prompt_internal.h>

#if defined(PLATFORM_LINUX)

#include <errno.h>

int prompt_platform_is_tty(void)
{
    return isatty(STDIN_FILENO);
}

void prompt_platform_enter_raw(com_util_prompt_t *p)
{
    struct termios raw;
    if (p->raw_active)
    {
        return;
    }
    if (tcgetattr(STDIN_FILENO, &p->orig_term) != 0)
    {
        return;
    }
    raw = p->orig_term;

    /* ICANON : 行単位読み取り無効（文字単位に変更）
     * ECHO   : エコー無効（自前で再描画）
     * ISIG   : Ctrl+C をシグナルではなく KEY_CTRL_C として受け取る
     * ICRNL  : \r を \n に変換しない
     * IXON   : Ctrl+S/Q による出力一時停止を無効 */
    raw.c_lflag &= ~((tcflag_t)(ICANON | ECHO | ISIG));
    raw.c_iflag &= ~((tcflag_t)(ICRNL | IXON));
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0)
    {
        p->raw_active = 1;
    }
}

void prompt_platform_leave_raw(com_util_prompt_t *p)
{
    if (!p->raw_active)
    {
        return;
    }
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &p->orig_term);
    p->raw_active = 0;
}

int prompt_platform_read_char(com_util_prompt_t *p)
{
    unsigned char c;
    ssize_t n;
    (void)p;

    do
    {
        n = read(STDIN_FILENO, &c, 1);
    } while (n < 0 && errno == EINTR);

    if (n == 1)
    {
        return (int)c;
    }
    else
    {
        return -1;
    }
}

int prompt_platform_read_char_nb(com_util_prompt_t *p)
{
    fd_set fds;
    struct timeval tv;
    (void)p;

    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    tv.tv_sec  = 0;
    tv.tv_usec = 50000; /* 50ms */

    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) <= 0)
    {
        return -1;
    }
    return prompt_platform_read_char(p);
}

#endif /* PLATFORM_LINUX */
