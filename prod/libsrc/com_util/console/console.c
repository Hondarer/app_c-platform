/**
 *  @file           console.c
 *  @brief          コンソール UTF-8 ヘルパー実装。
 *  @details        Windows 環境: stdout / stderr を匿名パイプに差し替え、
 *                  読み取りスレッドがコンソール (TTY) には WriteConsoleW で UTF-16 として
 *                  出力し、パイプやファイルへは UTF-8 バイト列をそのまま転送します。\n
 *                  Linux 環境: com_util_console_init / com_util_console_dispose は no-op です。
 */

#include <com_util/console/console.h>
#include <com_util/console/console_internal.h>

/* ===== Windows 実装 ===== */

#if defined(PLATFORM_WINDOWS)

#include <com_util/base/windows_sdk.h>
#include <com_util/sync/sync.h>
#include <io.h>      /* _open_osfhandle, _dup, _dup2, _close, _fileno */
#include <fcntl.h>   /* _O_WRONLY, _O_BINARY */
#include <stdio.h>   /* FILE, setvbuf, fflush, stdout, stderr */
#include <string.h>  /* memmove */

/* パイプ読み取りスレッドのバッファサイズ (バイト) */
#define READ_BUF_SIZE 4096

/* ストリームごとの内部状態 */
typedef struct {
    HANDLE orig_handle; /* DuplicateHandle で保存した元の Windows ハンドル */
    HANDLE pipe_read;   /* 匿名パイプの読み取り端 */
    int    orig_crt_fd; /* _dup で保存した元の CRT ファイルディスクリプタ */
    com_util_thread_t thread; /* 読み取りスレッドハンドル */
    LONG   active;      /* 初期化済みフラグ (InterlockedExchange で操作) */
} stream_state_t;

/* グローバル状態 (ゼロ初期化) */
static stream_state_t s_stdout_state;
static stream_state_t s_stderr_state;

/* ===== UTF-8 境界ユーティリティ ===== */

/*
 * buf[0..len) の末尾にある不完全な UTF-8 マルチバイトシーケンスを除いた
 * 有効バイト数を返す。buf 全体が完全なシーケンスで終端している場合は len を返す。
 *
 * UTF-8 エンコーディング:
 *   0xxxxxxx : 1 バイト (ASCII)
 *   110xxxxx : 2 バイト先頭
 *   1110xxxx : 3 バイト先頭
 *   11110xxx : 4 バイト先頭
 *   10xxxxxx : 継続バイト
 */
static int utf8_complete_length(const char *buf, int len)
{
    int i;
    unsigned char lead;
    int seq_len;

    if (len == 0) return 0;

    /* 末尾から最後の非継続バイト (リードバイトまたは ASCII) を探す */
    i = len - 1;
    while (i >= 0 && ((unsigned char)buf[i] & 0xC0) == 0x80) {
        i--;
    }
    if (i < 0) return 0; /* 全バイトが継続バイト (異常データ): 安全のため 0 を返す */

    lead = (unsigned char)buf[i];
    if      ((lead & 0x80) == 0x00) seq_len = 1; /* ASCII */
    else if ((lead & 0xE0) == 0xC0) seq_len = 2;
    else if ((lead & 0xF0) == 0xE0) seq_len = 3;
    else if ((lead & 0xF8) == 0xF0) seq_len = 4;
    else return len; /* 不正なリードバイト: 破損データとして境界を変更しない */

    /* シーケンスが完結しているか確認 */
    return (len - i >= seq_len) ? len : i;
}

/* ===== 読み取りスレッド ===== */

/*
 * パイプから UTF-8 バイト列を読み取り、出力先に応じて書き出す。
 *   - コンソール (TTY): WriteConsoleW で UTF-16 として出力。失敗時は WriteFile でフォールバック。
 *   - パイプ / ファイル: WriteFile で UTF-8 バイト列をそのまま転送。
 * マルチバイト文字の途中で分割された場合は次回の読み取りまで末尾バイトを保留する。
 */
static void reader_thread_proc(void *param)
{
    stream_state_t *s = (stream_state_t *)param;
    char    buf[READ_BUF_SIZE];
    wchar_t wbuf[READ_BUF_SIZE]; /* UTF-16 変換バッファ (最大 READ_BUF_SIZE 文字) */
    int     pending_len = 0;
    DWORD   mode        = 0;
    BOOL    is_console  = (GetFileType(s->orig_handle) == FILE_TYPE_CHAR)
                       && GetConsoleMode(s->orig_handle, &mode);

    for (;;) {
        DWORD nread = 0;
        BOOL  ok    = ReadFile(s->pipe_read,
                               buf + pending_len,
                               (DWORD)(READ_BUF_SIZE - pending_len),
                               &nread, NULL);
        if (!ok || nread == 0) break; /* パイプが閉じた / エラー */

        {
            int total = pending_len + (int)nread;

            if (is_console) {
                /* 完全な UTF-8 シーケンスの境界を計算し、不完全な末尾を保留する */
                int complete   = utf8_complete_length(buf, total);
                int new_pending = total - complete;

                if (complete > 0) {
                    int wlen = MultiByteToWideChar(CP_UTF8, 0,
                                                   buf, complete,
                                                   wbuf, READ_BUF_SIZE);
                    if (wlen > 0) {
                        DWORD nw = 0;
                        if (!WriteConsoleW(s->orig_handle, wbuf, (DWORD)wlen, &nw, NULL)) {
                            /* WriteConsoleW 失敗: UTF-8 バイト列をそのまま書き戻す */
                            WriteFile(s->orig_handle, buf, (DWORD)complete, &nw, NULL);
                        }
                    }
                }

                /* 不完全な末尾シーケンスを buf 先頭に移動して次回へ持ち越す */
                if (new_pending > 0) {
                    memmove(buf, buf + complete, (size_t)new_pending);
                }
                pending_len = new_pending;
            } else {
                /* パイプ / ファイル: UTF-8 バイト列をそのまま転送 */
                DWORD nw = 0;
                WriteFile(s->orig_handle, buf, (DWORD)total, &nw, NULL);
                pending_len = 0;
            }
        }
    }

    return;
}

/* ===== ストリーム初期化 / 解放 ===== */

/*
 * 指定したストリームを匿名パイプに差し替え、読み取りスレッドを起動する。
 *
 * 処理順:
 *   1. GetStdHandle → DuplicateHandle で元ハンドルを保存
 *   2. CreatePipe で匿名パイプ (read, write) を作成
 *   3. _dup で元 CRT FD を保存
 *   4. _open_osfhandle + _dup2 で crt_stream を書き込み端に向ける
 *   5. setvbuf で CRT バッファを無効化
 *   6. com_util_thread_create で読み取りスレッドを起動
 */
static int init_stream(stream_state_t *s, DWORD std_handle_id, FILE *crt_stream)
{
    HANDLE proc;
    HANDLE orig;
    HANDLE pipe_read;
    HANDLE pipe_write;
    SECURITY_ATTRIBUTES sa;
    int    new_fd;

    orig = GetStdHandle(std_handle_id);
    if (orig == NULL || orig == INVALID_HANDLE_VALUE) return -1;

    /* 元ハンドルを安全に複製して保存 */
    proc = GetCurrentProcess();
    if (!DuplicateHandle(proc, orig, proc, &s->orig_handle,
                         0, FALSE, DUPLICATE_SAME_ACCESS)) return -1;

    /* 匿名パイプを作成 (書き込み端は継承可能に設定) */
    sa.nLength              = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle       = TRUE;
    if (!CreatePipe(&pipe_read, &pipe_write, &sa, 0)) {
        CloseHandle(s->orig_handle);
        return -1;
    }
    /* 読み取り端は子プロセスに継承しない */
    SetHandleInformation(pipe_read, HANDLE_FLAG_INHERIT, 0);
    s->pipe_read = pipe_read;

    /* 元 CRT FD を保存 */
    s->orig_crt_fd = _dup(_fileno(crt_stream));
    if (s->orig_crt_fd == -1) {
        CloseHandle(s->orig_handle);
        CloseHandle(pipe_read);
        CloseHandle(pipe_write);
        return -1;
    }

    /* パイプ書き込み端から CRT FD を作成し、stdout / stderr を差し替える */
    new_fd = _open_osfhandle((intptr_t)pipe_write, _O_WRONLY | _O_BINARY);
    if (new_fd == -1) {
        _close(s->orig_crt_fd);
        CloseHandle(s->orig_handle);
        CloseHandle(pipe_read);
        CloseHandle(pipe_write);
        return -1;
    }

    if (_dup2(new_fd, _fileno(crt_stream)) != 0) {
        _close(new_fd); /* pipe_write を解放 (_open_osfhandle が所有) */
        _close(s->orig_crt_fd);
        CloseHandle(s->orig_handle);
        CloseHandle(pipe_read);
        return -1;
    }
    _close(new_fd); /* _dup2 済み。pipe_write の元ハンドルを解放。FD 1 は独立した複製を保持。 */
    setvbuf(crt_stream, NULL, _IONBF, 0);

    /* 読み取りスレッドを起動 */
    if (com_util_thread_create(&s->thread, reader_thread_proc, s) != 0) {
        /* 差し替えを元に戻す (_dup2 で書き込み端の複製も閉じる) */
        _dup2(s->orig_crt_fd, _fileno(crt_stream));
        _close(s->orig_crt_fd);
        CloseHandle(s->orig_handle);
        CloseHandle(pipe_read);
        return -1;
    }

    InterlockedExchange(&s->active, 1);
    return 0;
}

/*
 * 指定したストリームを元のハンドルに戻し、読み取りスレッドを停止する。
 * active が 0 の場合 (未初期化) は何もしない。
 *
 * 処理順:
 *   1. fflush で CRT バッファを吐き出す
 *   2. _dup2 で crt_stream を元の FD に戻す → パイプ書き込み端を閉じて EOF を送出
 *   3. com_util_thread_join_timed でスレッド終了を待つ
 *   4. ハンドルを解放する
 */
static void dispose_stream(stream_state_t *s, FILE *crt_stream)
{
    /* active を 1 → 0 に変更。戻り値が 0 なら元々 inactive なので何もしない。 */
    if (!InterlockedCompareExchange(&s->active, 0, 1)) return;

    fflush(crt_stream);

    /* crt_stream を元の FD に戻す。これにより FD 1 / 2 の書き込み端複製が閉じ、
     * スレッドの ReadFile が ERROR_BROKEN_PIPE で返る (EOF 相当)。 */
    _dup2(s->orig_crt_fd, _fileno(crt_stream));
    _close(s->orig_crt_fd);

    /* スレッド終了を最大 5 秒待つ */
    if (com_util_thread_join_timed(&s->thread, 5000) != 0)
    {
        com_util_thread_detach(&s->thread);
    }

    CloseHandle(s->pipe_read);
    CloseHandle(s->orig_handle);

    s->orig_crt_fd = -1;
    s->pipe_read   = NULL;
    s->orig_handle = NULL;
    s->thread      = NULL;
}

/*
 * DLL アンロードコンテキスト向けのストリーム解放関数。
 * dispose_stream との違い: com_util_thread_join_timed のタイムアウトを 500ms に短縮。
 * プロセス終了時 (process_terminating=1) は呼び出し元がスキップするため不要。
 *
 * パイプ閉鎖後に reader スレッドが ReadFile から返るまでの時間は通常ごく短い。
 * reader_thread_proc は LoadLibrary 等ローダーロックを要求する操作を行わないため、
 * DllMain コンテキストでも 500ms 待機によるデッドロックリスクはない。
 */
static void dispose_stream_on_unload(stream_state_t *s, FILE *crt_stream)
{
    if (!InterlockedCompareExchange(&s->active, 0, 1)) return;

    fflush(crt_stream);
    _dup2(s->orig_crt_fd, _fileno(crt_stream));
    _close(s->orig_crt_fd);

    if (com_util_thread_join_timed(&s->thread, 500) != 0)
    {
        com_util_thread_detach(&s->thread);
    }

    CloseHandle(s->pipe_read);
    CloseHandle(s->orig_handle);

    s->orig_crt_fd = -1;
    s->pipe_read   = NULL;
    s->orig_handle = NULL;
    s->thread      = NULL;
}

/* ===== 公開 API ===== */

COM_UTIL_EXPORT void COM_UTIL_API com_util_console_init(void)
{
    /* 二重初期化を防ぐ (マルチスレッド安全ではないが init はシングルスレッドで呼ぶ想定) */
    if (s_stdout_state.active) return;

    /* stdout がコンソール (TTY) でなければ内部パイプへの差し替えは不要。
     * パイプ/ファイル出力時は UTF-8 バイト列をそのまま転送できる。
     * 差し替えを行うと com_util_console_dispose() を呼ばずに return した場合に
     * リーダースレッドが ExitProcess() で強制終了されデータが失われる。 */
    {
        HANDLE h    = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD  mode = 0;
        if (GetFileType(h) != FILE_TYPE_CHAR || !GetConsoleMode(h, &mode))
            return;
    }

    if (init_stream(&s_stdout_state, STD_OUTPUT_HANDLE, stdout) != 0)
    {
        fprintf(stderr, "com_util_console_init: stdout の初期化に失敗しました。\n");
        return;
    }

    if (init_stream(&s_stderr_state, STD_ERROR_HANDLE, stderr) != 0) {
        /* stderr の初期化に失敗した場合は stdout の差し替えをロールバックする */
        dispose_stream(&s_stdout_state, stdout);
        fprintf(stderr, "com_util_console_init: stderr の初期化に失敗しました。\n");
        return;
    }
}

COM_UTIL_EXPORT void COM_UTIL_API com_util_console_dispose(void)
{
    /* stderr → stdout の順で解放する */
    dispose_stream(&s_stderr_state, stderr);
    dispose_stream(&s_stdout_state, stdout);
}

void com_util_console_dispose_on_unload(int process_terminating)
{
    /* プロセス終了時は OS がスレッドを自動終了・リソースを回収する。
     * ローダーロック保持中のスレッド待機はデッドロックになりうるため何もしない。 */
    if (process_terminating) return;
    /* stderr → stdout の順で解放する */
    dispose_stream_on_unload(&s_stderr_state, stderr);
    dispose_stream_on_unload(&s_stdout_state, stdout);
}

#elif defined(PLATFORM_LINUX)

/* ===== Linux 実装 (no-op) ===== */

COM_UTIL_EXPORT void COM_UTIL_API com_util_console_init(void)    {}
COM_UTIL_EXPORT void COM_UTIL_API com_util_console_dispose(void) {}
void com_util_console_dispose_on_unload(int process_terminating) { (void)process_terminating; }

#endif /* PLATFORM_ */
