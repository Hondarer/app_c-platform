/**
 *  @file           console.c
 *  @brief          Windows コンソール設定ヘルパー実装。
 *
 *  Windows 環境: 接続先コンソールの入出力コード ページを UTF-8 に設定し、
 *  stdout / stderr の Virtual Terminal Processing を有効化します。\n
 *  Linux 環境: com_util_console_init / com_util_console_dispose は no-op です。
 */

#include <com_util/console/console.h>
#include <com_util/console/console_internal.h>
#include <com_util/crt/unistd.h>
#include <com_util/runtime/shutdown.h>

/* ===== Windows 実装 ===== */

#if defined(PLATFORM_WINDOWS)

    #include <com_util/base/windows_sdk.h>
    #include <com_util/crt/stdio.h>
    #include <com_util/sync/sync.h>
    #include <stdint.h> /* uintptr_t */
    #include <stdio.h>  /* stdout, stderr */
    #include <stdlib.h> /* strtoul, strtoull */
    #include <string.h> /* strncmp, strlen */

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
    if (!com_util_isatty(COM_UTIL_STREAM_STDOUT))
    {
        InterlockedExchange(&s_initialized, 0);
        return;
    }

    /* コンソールの入出力コード ページを UTF-8 に設定 (すでに UTF-8 なら変更しない) */
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

    /* Virtual Terminal Processing を有効化 (ANSI エスケープ シーケンス対応) */
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

    /* コンソール モードを元に戻す */
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

    /* コード ページを元に戻す */
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

/**
 *  @brief          argv から親コンソール引き継ぎフラグを取り出して除去する。
 *  @param[in,out]  argc     引数の数へのポインター。
 *  @param[in,out]  argv     引数配列。
 *  @param[out]     out_pid     取り出した親プロセス ID の格納先。
 *  @param[out]     out_window  取り出した親コンソール window ハンドルの格納先 (省略時は NULL)。
 *  @return         有効なフラグを検出した場合は 1、そうでない場合は 0 を返します。
 *
 *  フラグの値は `<PID>` または `<PID>:<HWND>` 形式です。@p out_window には HWND を
 *  復元して格納し、HWND が無い・不正な場合は NULL を格納します。\n
 *  フラグを検出した場合は、値の解析可否にかかわらず @p argv から取り除き、
 *  @p argc を 1 減らします。PID が不正な場合は 0 を返します。
 */
static int extract_handover_args(int *argc, char **argv, DWORD *out_pid, HWND *out_window)
{
    const char *prefix = COM_UTIL_CONSOLE_HANDOVER_FLAG "=";
    size_t prefix_len;
    int n;
    int i;
    int found;
    DWORD pid;
    HWND window;

    if (argc == NULL || argv == NULL || out_pid == NULL || out_window == NULL)
    {
        return 0;
    }

    prefix_len = strlen(prefix);
    n = *argc;
    found = 0;
    pid = 0;
    window = NULL;

    for (i = 1; i < n; i++)
    {
        char *endp;
        unsigned long value;
        int j;

        if (argv[i] == NULL || strncmp(argv[i], prefix, prefix_len) != 0)
        {
            continue;
        }

        endp = NULL;
        value = strtoul(argv[i] + prefix_len, &endp, 10);
        if (endp != argv[i] + prefix_len && (*endp == '\0' || *endp == ':') && value != 0)
        {
            pid = (DWORD)value;
            found = 1;

            /* 区切り ':' に続く HWND を任意で解析する (旧形式の PID のみでも受理する) */
            if (*endp == ':')
            {
                const char *window_text = endp + 1;
                char *window_endp = NULL;
                unsigned long long window_value;

                window_value = strtoull(window_text, &window_endp, 10);
                if (window_endp != window_text && *window_endp == '\0')
                {
                    window = (HWND)(uintptr_t)window_value;
                }
            }
        }

        /* フラグを取り除いて後続を前へ詰める (不正値でも除去する) */
        for (j = i; j < n - 1; j++)
        {
            argv[j] = argv[j + 1];
        }
        argv[n - 1] = NULL;
        *argc = n - 1;
        break;
    }

    if (found)
    {
        *out_pid = pid;
        *out_window = window;
    }
    return found;
}

COM_UTIL_EXPORT int COM_UTIL_API com_util_console_attach_parent(int *argc, char **argv)
{
    DWORD parent_pid;
    HWND parent_window;
    HANDLE h_out;
    HANDLE h_err;
    HANDLE h_in;
    int attached;
    int attempt;

    parent_pid = 0;
    parent_window = NULL;
    if (!extract_handover_args(argc, argv, &parent_pid, &parent_window))
    {
        return 0;
    }

    /* 昇格時に割り当てられた一時コンソールを切り離し、親コンソールへ接続する。
       昇格直後は子の一時コンソール (conhost) 割り当てが非同期に進むため、
       自前コンソールへ繋がったままだと AttachConsole が ERROR_ACCESS_DENIED で
       失敗することがある。割り当てが落ち着くまで有界リトライする。\n
       さらに、AttachConsole が成功しても親コンソールの window ハンドルが取得できる
       状態になるまでには間がある。親 HWND が判っている場合は GetConsoleWindow() が
       親 HWND に一致するまで待ち、一時コンソールへ出力が吸われるのを防ぐ。
       全試行で一致しない場合は、素の AttachConsole 成功を受理してフォールバックする。 */
    attached = 0;
    for (attempt = 0; attempt < COM_UTIL_CONSOLE_ATTACH_MAX_ATTEMPTS; attempt++)
    {
        FreeConsole();
        if (AttachConsole(parent_pid))
        {
            if (parent_window == NULL || GetConsoleWindow() == parent_window)
            {
                attached = 1;
                break;
            }
        }
        Sleep(COM_UTIL_CONSOLE_ATTACH_RETRY_INTERVAL_MS);
    }
    if (attached == 0 && GetConsoleWindow() == NULL)
    {
        /* HWND 一致が得られず、かつどのコンソールにも繋がっていない場合のみ失敗とする。
           AttachConsole 自体は成功しているがウインドウ一致だけ得られない場合は、
           従来動作を下限として付け替えを続行する。 */
        return -1;
    }

    /* 親コンソールの出力バッファーが使用可能になるまで待つ。AttachConsole 成功直後は
       conhost の再割り当てが落ち着いておらず、直後の CONOUT$ 書き込みが消えかけの
       一時コンソールへ吸われて画面に出ないことがある。GetConsoleScreenBufferInfo() が
       成功する (= 出力可能になった) のを有界リトライで確認してからハンドルを付け替える。 */
    for (attempt = 0; attempt < COM_UTIL_CONSOLE_ATTACH_MAX_ATTEMPTS; attempt++)
    {
        HANDLE probe;
        CONSOLE_SCREEN_BUFFER_INFO info;
        int ready;

        probe = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                            OPEN_EXISTING, 0, NULL);
        ready = 0;
        if (probe != INVALID_HANDLE_VALUE)
        {
            if (GetConsoleScreenBufferInfo(probe, &info))
            {
                ready = 1;
            }
            CloseHandle(probe);
        }
        if (ready != 0)
        {
            break;
        }
        Sleep(COM_UTIL_CONSOLE_ATTACH_RETRY_INTERVAL_MS);
    }

    /* Win32 レベルの標準ハンドルを親コンソールへ付け替える
       (GetStdHandle / WriteConsole 系や tracer の stderr sink が参照する) */
    h_out = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, 0, NULL);
    if (h_out != INVALID_HANDLE_VALUE)
    {
        SetStdHandle(STD_OUTPUT_HANDLE, h_out);
    }
    h_err = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, 0, NULL);
    if (h_err != INVALID_HANDLE_VALUE)
    {
        SetStdHandle(STD_ERROR_HANDLE, h_err);
    }
    h_in = CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                       0, NULL);
    if (h_in != INVALID_HANDLE_VALUE)
    {
        SetStdHandle(STD_INPUT_HANDLE, h_in);
    }

    /* CRT レベルの標準ストリームを親コンソールへ再接続する
       (printf / fprintf 系が参照する) */
    if (com_util_freopen("CONOUT$", "w", stdout, NULL) == NULL)
    {
        /* 失敗しても Win32 ハンドルは付け替え済みのため処理を継続する */
    }
    if (com_util_freopen("CONOUT$", "w", stderr, NULL) == NULL)
    {
        /* 失敗しても Win32 ハンドルは付け替え済みのため処理を継続する */
    }
    if (com_util_freopen("CONIN$", "r", stdin, NULL) == NULL)
    {
        /* 失敗しても Win32 ハンドルは付け替え済みのため処理を継続する */
    }

    return 1;
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
COM_UTIL_EXPORT int COM_UTIL_API com_util_console_attach_parent(int *argc, char **argv)
{
    (void)argc;
    (void)argv;
    return 0;
}
void com_util_console_dispose_on_shutdown(const com_util_shutdown_event *event, void *context)
{
    (void)event;
    (void)context;
}

#endif /* PLATFORM_ */
