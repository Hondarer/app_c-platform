/**
 *  @file           console.h
 *  @brief          Windows コンソール設定ヘルパー API。
 *
 *  Windows 環境で接続先コンソールの入出力コードページを UTF-8 に設定し、
 *  stdout / stderr の Virtual Terminal Processing を有効化します。\n
 *  Linux 環境では @c com_util_console_init は何もしません。\n
 *  呼び出し側は @c \#ifdef @c _WIN32 ガード不要でクロスプラットフォームに
 *  使用できます。
 *
 *  @par            使用例
 *  @code{.c}
    #include <com_util/console/console.h>
    #include <stdio.h>

    int main(void) {
        com_util_console_init();     // Windows コンソール設定を初期化
        printf("こんにちは\n");
        fprintf(stderr, "警告\n");
        com_util_console_dispose();
        return 0;
    }
 *  @endcode
 *
 *  @hideincludedbygraph
 *
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_CONSOLE_H
#define COM_UTIL_CONSOLE_H

#include <com_util/base/platform.h>
#include <com_util/com_util_export.h>


/**
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @{
 */

/* ===== API 関数 ===== */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          コンソールヘルパーを初期化する。
     *
     *  Windows 環境では stdout がコンソール (TTY) の場合に、
     *  コンソール入出力コードページを UTF-8 に設定し、
     *  stdout / stderr の Virtual Terminal Processing を有効化します。\n
     *  Linux 環境では何もしません。\n
     *  本関数はプログラム開始時に一度だけ呼び出すことを想定しています。\n
     *  stdout がコンソールでない場合、またはコンソール情報を取得できない場合は
     *  何もせずに返ります。
     *
     *  @note           初回利用時に shutdown コールバックが自動登録されます。\n
     *                  明示的に解放する場合は @c com_util_console_dispose を呼び出してください。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  プロセス起動直後のシングル スレッド フェーズで 1 度だけ呼び出してください。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_console_init(void);

    /**
     *  @brief          コンソールヘルパーを終了し、リソースを解放する。
     *
     *  Windows 環境では @c com_util_console_init で変更した
     *  コンソール入出力コードページとコンソール モードを元に戻します。\n
     *  Linux 環境では何もしません。\n
     *                  @c com_util_console_init を呼び出していない場合も安全に呼び出せます。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  プロセス終了直前のシングル スレッド フェーズで呼び出してください。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_console_dispose(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_CONSOLE_H */
