/**
 *******************************************************************************
 *  @file           elevated_process.h
 *  @brief          管理者権限を確認し、昇格プロセスを起動する API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/06/20
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_ELEVATED_PROCESS_H
#define COM_UTIL_ELEVATED_PROCESS_H

#include <stddef.h>
#include <com_util/base/platform.h>
#include <com_util/com_util_export.h>

/**
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          現在のプロセスが管理者/root 権限で動作しているかを確認します。
     *  @param[out]     elevated  昇格済みなら 1、そうでなければ 0 の格納先。NULL を渡してはなりません。
     *  @return         確認に成功した場合は 0、失敗した場合は -1 を返します。
     *
     *  Windows ではプロセス トークンの昇格状態 (TokenElevation) を確認します。\n
     *  Linux では実効ユーザー ID が root (0) かどうかを確認します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_elevated_process_is_elevated(int *elevated);

    /**
     *  @brief          管理者/root 権限が必要な処理のため、必要に応じて昇格実行します。
     *  @param[in]      arguments  昇格実行時に現在の実行ファイルへ渡す引数文字列。NULL 可。
     *  @param[out]     exit_code  昇格プロセスの終了コード、または 0 の格納先。
     *  @param[out]     handled    昇格プロセスで処理した場合は 0 以外、現プロセスで継続する場合は 0 の格納先。
     *  @return         権限確認または昇格プロセス実行が成功した場合は 0、失敗した場合は -1 を返します。
     *
     *  Windows では、未昇格の場合に UAC を要求して現在の実行ファイルを @p arguments 付きで
     *  再起動し、子プロセスの終了まで待機します。すでに昇格済みの場合は何もしません。\n
     *  未昇格かつセッション 0 (非対話セッション。Windows サービスなど、UAC ダイアログを
     *  表示できる対話デスクトップを持たないセッション) の場合は、昇格を試みず失敗を返します。\n
     *  Linux では、実効ユーザー ID が root であることを確認します。root でない場合は失敗します。\n
     *  本関数は権限保証のための API であり、Linux で `sudo` などの外部昇格コマンドは実行しません。
     *
     *  @note           Windows で呼び出し元にコンソールがある場合、昇格プロセスへ親コンソールを
     *                  引き継ぐため、昇格プロセスは別ウインドウを表示せず親コンソールへ出力します。\n
     *                  この場合 @p arguments の末尾に内部フラグを付与して再起動するため、昇格
     *                  プロセスは起動直後に com_util_console_attach_parent() を呼び出す必要が
     *                  あります。呼び出さない場合、昇格プロセスの出力は表示されません。\n
     *                  呼び出し元にコンソールが無い場合は、昇格プロセスを通常表示で起動します。
     *
     *  @par            スレッド セーフ
     *  本関数は内部に共有状態を持ちません。ただし、Windows で UAC を表示するため、
     *  通常はメイン スレッドまたはユーザー操作に応答するスレッドから呼び出してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_elevated_process_run_if_needed(const char *arguments, int *exit_code,
                                                                             int *handled);

    /**
     *  @brief          管理者/root 権限が必要な処理のため、必要に応じて昇格実行し、
     *                  昇格プロセスが報告した結果メッセージを取得します。
     *  @param[in]      arguments            昇格実行時に現在の実行ファイルへ渡す引数文字列。NULL 可。
     *  @param[out]     exit_code            昇格プロセスの終了コード、または 0 の格納先。
     *  @param[out]     handled              昇格プロセスで処理した場合は 0 以外、現プロセスで継続する場合は 0 の格納先。
     *  @param[out]     result_message       昇格プロセスが報告したメッセージ (UTF-8) の格納先。NULL 可。
     *  @param[in]      result_message_size  @p result_message のバイト数。
     *  @return         権限確認または昇格プロセス実行が成功した場合は 0、失敗した場合は -1 を返します。
     *
     *  com_util_elevated_process_run_if_needed() はコンソールの再接続を昇格プロセス側に
     *  要求しますが、UAC 昇格直後の親コンソール再割り当ては実機調査でも原因を特定できない
     *  間欠的な書き込み不能 (`ERROR_INVALID_HANDLE`) を起こすことが分かっています。\n
     *  本関数は昇格プロセスのコンソールを一切引き継がず、結果メッセージを一時ファイル経由で
     *  受け渡します。昇格プロセス側は com_util_elevated_process_extract_result_target() を起動直後に
     *  呼び出し、処理結果を com_util_elevated_process_report_result() で報告してください。\n
     *  com_util_elevated_process_run_if_needed() と同様、未昇格かつセッション 0 (非対話
     *  セッション) の場合は、昇格を試みず失敗を返します。\n
     *  Linux では com_util_elevated_process_run_if_needed() と同じ判定を行い、
     *  @p result_message は変更しません (別プロセスを起動しないため報告の余地がない)。
     *
     *  @par            スレッド セーフ
     *  本関数は内部に共有状態を持ちません。ただし、Windows で UAC を表示するため、
     *  通常はメイン スレッドまたはユーザー操作に応答するスレッドから呼び出してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_elevated_process_run_with_result(const char *arguments, int *exit_code,
                                                                               int *handled, char *result_message,
                                                                               size_t result_message_size);

    /**
     *  @brief          argv から結果報告先フラグを取り出します。
     *  @param[in,out]  argc  引数の数へのポインター。NULL 可。
     *  @param[in,out]  argv  引数配列。NULL 可。
     *  @return         報告先フラグを検出した場合は 1、そうでない場合は 0 を返します。
     *
     *  com_util_elevated_process_run_with_result() が付与したフラグを検出し、後続の
     *  com_util_elevated_process_report_result() のために報告先を保持します。検出した
     *  フラグは @p argv から取り除き、@p argc を 1 減らします。\n
     *  プログラム開始直後、引数解析より前に 1 度だけ呼び出してください。\n
     *  Linux では何もせず 0 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  プロセス起動直後のシングル スレッド フェーズで呼び出してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_elevated_process_extract_result_target(int *argc, char **argv);

    /**
     *  @brief          昇格プロセスから、呼び出し元プロセスへ結果メッセージを報告します。
     *  @param[in]      message  報告するメッセージ (UTF-8)。NULL を渡してはなりません。
     *  @return         成功時は 0、報告先が無い場合や失敗時は -1 を返します。
     *
     *  com_util_elevated_process_extract_result_target() で報告先を検出している場合のみ、
     *  そのファイルへ @p message を書き込みます。報告先を検出していない場合
     *  (com_util_elevated_process_run_with_result() 経由で起動されていない場合) は
     *  何も行わず -1 を返すため、呼び出し元はその場合に自分自身の標準出力へ
     *  直接表示してください。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  内部状態は com_util_elevated_process_extract_result_target() が設定したものを参照します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_elevated_process_report_result(const char *message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_ELEVATED_PROCESS_H */
