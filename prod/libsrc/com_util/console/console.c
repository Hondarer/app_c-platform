/**
 *  @file           console.c
 *  @brief          Windows コンソール設定ヘルパー実装。
 *
 *  Windows 環境: 接続先コンソールの入出力コードページを UTF-8 に設定し、
 *  stdout / stderr の Virtual Terminal Processing を有効化します。\n
 *  Linux 環境: com_util_console_init / com_util_console_dispose は no-op です。
 */

#include <com_util/console/console.h>
#include <com_util/console/console_internal.h>
#include <com_util/runtime/shutdown.h>

/* ===== Windows 実装 ===== */

#if defined(PLATFORM_WINDOWS)

    #include <com_util/base/windows_sdk.h>
    #include <com_util/sync/sync.h>
    #include <stdio.h> /* stdout, stderr */

/* 初期化前のコンソール状態を保存 */
static UINT s_orig_output_cp = 0;
static UINT s_orig_input_cp = 0;
static DWORD s_orig_stdout_mode = 0;
static DWORD s_orig_stderr_mode = 0;
static LONG s_initialized = 0;
static com_util_once_flag s_console_shutdown_once = {0};

static void register_console_shutdown_callback(void)
{
    (void)com_util_shutdown_register(com_util_console_dispose_on_shutdown, NULL);
}

/* ===== 公開 API ===== */

COM_UTIL_EXPORT void COM_UTIL_API com_util_console_init(void)
{
    HANDLE h;
    DWORD mode = 0;
    UINT current_cp;

    com_util_call_once(&s_console_shutdown_once, register_console_shutdown_callback);

    /* 二重初期化を防ぐ */
    if (InterlockedCompareExchange(&s_initialized, 1, 0))
        return;

    /* stdout がコンソール (TTY) でなければ何もしない */
    h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetFileType(h) != FILE_TYPE_CHAR || !GetConsoleMode(h, &mode))
    {
        InterlockedExchange(&s_initialized, 0);
        return;
    }

    /* コンソールの入出力コードページを UTF-8 に設定 (既に UTF-8 なら変更しない) */
    current_cp = GetConsoleCP();
    if (current_cp != CP_UTF8)
    {
        s_orig_input_cp = current_cp;
        SetConsoleCP(CP_UTF8);
    }

    current_cp = GetConsoleOutputCP();
    if (current_cp != CP_UTF8)
    {
        s_orig_output_cp = current_cp;
        SetConsoleOutputCP(CP_UTF8);
    }

    /* Virtual Terminal Processing を有効化 (ANSI エスケープシーケンス対応) */
    h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleMode(h, &mode))
    {
        if (!(mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        {
            s_orig_stdout_mode = mode;
            SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }

    h = GetStdHandle(STD_ERROR_HANDLE);
    if (GetConsoleMode(h, &mode))
    {
        if (!(mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        {
            s_orig_stderr_mode = mode;
            SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
}

COM_UTIL_EXPORT void COM_UTIL_API com_util_console_dispose(void)
{
    HANDLE h;

    /* initialized を 1 → 0 に変更。戻り値が 0 なら元々未初期化なので何もしない。 */
    if (!InterlockedCompareExchange(&s_initialized, 0, 1))
        return;

    /* コンソールモードを元に戻す */
    if (s_orig_stdout_mode != 0)
    {
        h = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleMode(h, s_orig_stdout_mode);
        s_orig_stdout_mode = 0;
    }

    if (s_orig_stderr_mode != 0)
    {
        h = GetStdHandle(STD_ERROR_HANDLE);
        SetConsoleMode(h, s_orig_stderr_mode);
        s_orig_stderr_mode = 0;
    }

    /* コードページを元に戻す */
    if (s_orig_input_cp != 0)
    {
        SetConsoleCP(s_orig_input_cp);
        s_orig_input_cp = 0;
    }

    if (s_orig_output_cp != 0)
    {
        SetConsoleOutputCP(s_orig_output_cp);
        s_orig_output_cp = 0;
    }
}

void com_util_console_dispose_on_shutdown(const com_util_shutdown_event *event, void *context)
{
    (void)context;
    if (event == NULL || event->reason != COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT)
        return;

    /* stdout/stderr をフラッシュしてからコンソール状態を戻す */
    fflush(stdout);
    fflush(stderr);

    com_util_console_dispose();
}

#elif defined(PLATFORM_LINUX)

/* ===== Linux 実装 (no-op) ===== */

COM_UTIL_EXPORT void COM_UTIL_API com_util_console_init(void) {}
COM_UTIL_EXPORT void COM_UTIL_API com_util_console_dispose(void) {}
void com_util_console_dispose_on_shutdown(const com_util_shutdown_event *event, void *context)
{
    (void)event;
    (void)context;
}

#endif /* PLATFORM_ */
