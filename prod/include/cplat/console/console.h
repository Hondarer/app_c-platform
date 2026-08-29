/**
 *  @file           console.h
 *  @brief          Windows コンソールを設定するヘルパー API を提供します。
 *
 *  Windows 環境で接続先コンソールの入出力コード ページを UTF-8 に設定し、
 *  stdout / stderr の Virtual Terminal Processing を有効化します。\n
 *  Linux 環境では `cplat_console_init` は何もしません。\n
 *  呼び出し側は `#ifdef _WIN32` ガード不要でクロスプラットフォームに
 *  使用できます。
 *
 *  @par            使用例
    @code{.c}
    #include <cplat/console/console.h>
    #include <cplat/crt/stdio.h>
    #include <stdio.h>

    int main(void) {
        cplat_console_init();     // Windows コンソール設定を初期化
        printf("こんにちは\n");
        cplat_fprintf(stderr, "警告\n");
        cplat_console_dispose();
        return 0;
    }
    @endcode
 *
 *  @hideincludedbygraph
 *
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef CPLAT_CONSOLE_H
#define CPLAT_CONSOLE_H

#include <cplat/base/platform.h>
#include <cplat/base/result.h>
#include <cplat/cplat_export.h>
#include <cplat/crt/unistd.h>

/**
 *  @ingroup        CPLAT_CONSOLE
 *  @{
 */

/* ===== API 関数 ===== */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          コンソール ヘルパーを初期化します。
     *
     *  Windows 環境では stdout がコンソール (TTY) の場合に、
     *  コンソール入出力コード ページを UTF-8 に設定し、
     *  stdout / stderr の Virtual Terminal Processing を有効化します。\n
     *  Linux 環境では何もしません。\n
     *  本関数はプログラム開始時に一度だけ呼び出すことを想定しています。\n
     *  stdout がコンソールでない場合、またはコンソール情報を取得できない場合は
     *  何もせずに返ります。
     *
     *  @note           初回利用時に shutdown コールバックが自動登録されます。\n
     *                  明示的に解放する場合は `cplat_console_dispose` を呼び出してください。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  プロセス起動直後のシングル スレッド フェーズで 1 度だけ呼び出してください。
     */
    CPLAT_EXPORT void CPLAT_API cplat_console_init(void);

    /**
     *  @brief          コンソール ヘルパーを終了し、リソースを解放します。
     *
     *  Windows 環境では `cplat_console_init` で変更した
     *  コンソール入出力コード ページとコンソール モードを元に戻します。\n
     *  Linux 環境では何もしません。\n
     *                  `cplat_console_init` を呼び出していない場合も安全に呼び出せます。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  プロセス終了直前のシングル スレッド フェーズで呼び出してください。
     */
    CPLAT_EXPORT void CPLAT_API cplat_console_dispose(void);

    /**
     *  @brief          昇格起動時に親プロセスのコンソールへ再接続します。
     *  @param[in,out]  argc          引数の数へのポインター。NULL 可。
     *  @param[in,out]  argv          引数配列。NULL 可。
     *  @param[out]     attached_out  親コンソールへ再接続した場合は 1、何もしなかった場合は 0 の格納先。
     *                                NULL 可。戻り値が @ref CPLAT_OK の場合のみ有効です。
     *  @return         @ref CPLAT_OK 、@ref CPLAT_ERR_UNKNOWN のいずれかを返します。
     *
     *  Windows 環境では、cplat_elevated_process_run_if_needed() が UAC 昇格で
     *  自プロセスを再起動した際に付与する引き継ぎフラグを検出し、親プロセスの
     *  コンソールへ `AttachConsole` で再接続します。\n
     *  再接続後、stdin / stdout / stderr を親コンソール (CONIN$ / CONOUT$) へ
     *  つなぎ直すため、昇格プロセスの出力が元のコンソールにそのまま表示されます。\n
     *  検出した引き継ぎフラグは @p argv から取り除き、@p argc を 1 減らします。\n
     *  Linux 環境では何もせず @p attached_out に 0 を設定して @ref CPLAT_OK を返します。
     *
     *  @note           本関数はプログラム開始直後、引数解析および
     *                  `cplat_console_init` より前に 1 度だけ呼び出してください。\n
     *                  親プロセスにコンソールが無い場合 (GUI 起動など) は引き継ぎ
     *                  フラグが付与されないため、@p attached_out には 0 が設定されます。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  プロセス起動直後のシングル スレッド フェーズで呼び出してください。
     */
    CPLAT_EXPORT int CPLAT_API cplat_console_attach_parent(int *argc, char **argv, int *attached_out);

    /**
     *  @brief          stdout / stderr の CRT ストリーム (printf / fprintf) を経由せず、
     *                  端末へ文字列をそのまま書き込みます。
     *  @param[in]      stream  書き込み先 (@ref CPLAT_STREAM_STDOUT または
     *                  @ref CPLAT_STREAM_STDERR)。それ以外は失敗します。
     *  @param[in]      text    書き込む文字列 (UTF-8)。NULL を渡してはなりません。
     *  @return         @ref CPLAT_OK 、@ref CPLAT_ERR_INVALID_ARGUMENT 、@ref CPLAT_ERR_UNKNOWN のいずれかを返します。
     *
     *  Windows 環境では、cplat_console_attach_parent() による昇格後の親コンソール
     *  再接続直後に、stdout / stderr の CRT ストリーム (FILE*) 経由の printf / fprintf が
     *  fd 自体は正常であるにもかかわらず書き込みを拒否する事象が実機調査で確認されている。\n
     *  本関数は `GetStdHandle` で取得した Win32 ハンドルへ `WriteConsoleA` で直接書き込み、
     *  この問題を回避する。\n
     *  Linux 環境では対象の fd へ直接書き込みます。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    CPLAT_EXPORT int CPLAT_API cplat_console_write(cplat_stream stream, const char *text);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* CPLAT_CONSOLE_H */
