/**
 *******************************************************************************
 *  @file           pinned_prompt.h
 *  @brief          コマンド操作向けの固定プロンプト API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/05/08
 *  @version        0.1.0
 *
 *  端末の最下部に 1 行の入力プロンプトを固定し、アプリケーションの出力をその上へ表示します。
 *  本 API は実験段階であり、コマンド ライン操作モデルの改良に伴って変更される場合があります。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_PINNED_PROMPT_H
#define COM_UTIL_PINNED_PROMPT_H

#include <stddef.h>

#include <com_util/base/compiler.h>
#include <com_util/base/platform.h>
#include <com_util/base/result.h>
#include <com_util/prompt/prompt.h>
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
     *  @brief  固定プロンプトを操作する不透明ハンドルです。
     */
    typedef struct com_util_pinned_prompt com_util_pinned_prompt;

    /**
     *  @brief  固定プロンプトの上へ出力するときに使用する出力先です。
     */
    typedef enum
    {
        COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT = 0,
        COM_UTIL_PINNED_PROMPT_CHANNEL_STDERR = 1
    } com_util_pinned_prompt_channel_t;

    /**
     *  @brief  ステータス領域の表示位置です。
     */
    typedef enum
    {
        COM_UTIL_PINNED_PROMPT_STATUS_POSITION_TOP = 0,
        COM_UTIL_PINNED_PROMPT_STATUS_POSITION_BOTTOM = 1
    } com_util_pinned_prompt_status_position_t;

    /**
     *  @brief  ステータス領域内の文字列配置です。
     */
    typedef enum
    {
        COM_UTIL_PINNED_PROMPT_STATUS_ALIGN_LEFT = 0,
        COM_UTIL_PINNED_PROMPT_STATUS_ALIGN_RIGHT = 1
    } com_util_pinned_prompt_status_align_t;

    /**
     *  @brief  固定プロンプトの生成オプションです。
     */
    typedef struct com_util_pinned_prompt_options
    {
        /**
         *  @brief  将来拡張用のフラグです。0 を指定してください。
         */
        unsigned int flags;

        /**
         *  @brief  構造体配置用の予約領域です。0 を指定してください。
         */
        unsigned int reserved;

        /**
         *  @brief  入力編集と履歴に関するオプションです。
         */
        com_util_prompt_options input;
    } com_util_pinned_prompt_options;

    /**
     *  @brief          固定プロンプト ハンドルを生成します。
     *  @param[in]      options  生成オプションです。NULL の場合は既定値を使用します。
     *  @return         成功時は生成したハンドルを返します。メモリまたは同期オブジェクトを確保できない場合は
     *                  NULL を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。各呼び出しは独立したハンドルを生成します。
     */
    COM_UTIL_EXPORT com_util_pinned_prompt *COM_UTIL_API
    com_util_pinned_prompt_create(const com_util_pinned_prompt_options *options);

    /**
     *  @brief          固定プロンプト ハンドルを解放します。
     *  @param[in]      screen  com_util_pinned_prompt_create() が返したハンドルです。NULL も指定できます。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  解放対象の @p screen を他スレッドが使用していないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_pinned_prompt_dispose(com_util_pinned_prompt *screen);

/**
 *  @brief          端末下部に固定したプロンプトで 1 行のコマンド入力を受け取ります。
 *  @param[in]      screen      固定プロンプト ハンドルです。
 *  @param[out]     buf         入力結果を格納するバッファーです。終端の改行は格納しません。
 *  @param[in]      buf_size    @p buf のバイト数です。
 *  @param[in]      prompt_str  表示するプロンプト文字列です。NULL の場合は空文字列として扱います。
 *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_EOF 、@ref COM_UTIL_ERR_CANCELED 、
 *                  @ref COM_UTIL_ERR_INVALID_ARGUMENT 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
 */
#define com_util_pinned_prompt_readline(screen, buf, buf_size, prompt_str) \
    _com_util_pinned_prompt_readline((screen), (buf), (buf_size), (prompt_str), __FILE__, __LINE__)

/**
 *  @brief          書式指定した固定プロンプトで 1 行のコマンド入力を受け取ります。
 *  @param[in]      screen    固定プロンプト ハンドルです。
 *  @param[out]     buf       入力結果を格納するバッファーです。終端の改行は格納しません。
 *  @param[in]      buf_size  @p buf のバイト数です。
 *  @param[in]      fmt       printf 形式の書式文字列です。NULL の場合は空文字列として扱います。
 *  @param[in]      ...       @p fmt に対応する書式引数です。
 *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_EOF 、@ref COM_UTIL_ERR_CANCELED 、
 *                  @ref COM_UTIL_ERR_INVALID_ARGUMENT 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
 */
#define com_util_pinned_prompt_readline_fmt(screen, buf, buf_size, fmt, ...) \
    _com_util_pinned_prompt_readline_fmt((screen), (buf), (buf_size), __FILE__, __LINE__, (fmt), ##__VA_ARGS__)

    /**
     *  @brief          呼び出し元の位置を明示して 1 行のコマンド入力を受け取ります。
     *
     *  通常は com_util_pinned_prompt_readline() を使用してください。
     *
     *  @param[in]      screen      固定プロンプト ハンドルです。
     *  @param[out]     buf         入力結果を格納するバッファーです。
     *  @param[in]      buf_size    @p buf のバイト数です。
     *  @param[in]      prompt_str  表示するプロンプト文字列です。NULL の場合は空文字列として扱います。
     *  @param[in]      file        履歴を識別する呼び出し元ファイル名です。
     *  @param[in]      line        履歴を識別する呼び出し元行番号です。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_EOF 、@ref COM_UTIL_ERR_CANCELED 、
     *                  @ref COM_UTIL_ERR_INVALID_ARGUMENT 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  同一 @p screen への並行呼び出しは未定義動作です。入力は 1 スレッドから行ってください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API _com_util_pinned_prompt_readline(com_util_pinned_prompt *screen, char *buf,
                                                                      size_t buf_size, const char *prompt_str,
                                                                      const char *file, int line);

    /**
     *  @brief          呼び出し元の位置とプロンプト書式を明示してコマンド入力を受け取ります。
     *
     *  通常は com_util_pinned_prompt_readline_fmt() を使用してください。
     *
     *  @param[in]      screen    固定プロンプト ハンドルです。
     *  @param[out]     buf       入力結果を格納するバッファーです。
     *  @param[in]      buf_size  @p buf のバイト数です。
     *  @param[in]      file      履歴を識別する呼び出し元ファイル名です。
     *  @param[in]      line      履歴を識別する呼び出し元行番号です。
     *  @param[in]      fmt       printf 形式の書式文字列です。NULL の場合は空文字列として扱います。
     *  @param[in]      ...       @p fmt に対応する書式引数です。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_EOF 、@ref COM_UTIL_ERR_CANCELED 、
     *                  @ref COM_UTIL_ERR_INVALID_ARGUMENT 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API _com_util_pinned_prompt_readline_fmt(com_util_pinned_prompt *screen, char *buf,
                                                                          size_t buf_size, const char *file, int line,
                                                                          const char *fmt, ...)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 6, 7)))
#endif /* COMPILER_GCC */
        ;

    /**
     *  @brief          端末下部の固定プロンプトより上へデータを書き込みます。
     *  @param[in]      screen   固定プロンプト ハンドルです。
     *  @param[in]      channel  書き込み先の標準ストリームです。
     *  @param[in]      data     書き込むデータです。@p size が 0 の場合に限り NULL も指定できます。
     *  @param[in]      size     @p data から書き込むバイト数です。
     *  @return         対象ストリームへ書き込んだバイト数を返します。引数不正の場合は 0 を返します。
     *
     *  指定されたデータだけを書き込み、改行は付加しません。
     *  ANSI CSI SGR エスケープ シーケンスは、色指定としてそのまま出力します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部のミューテックスで保護されており、同一 @p screen に対して複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT size_t COM_UTIL_API com_util_pinned_prompt_write(com_util_pinned_prompt *screen,
                                                                     com_util_pinned_prompt_channel_t channel,
                                                                     const void *data, size_t size);

    /**
     *  @brief          端末下部の固定プロンプトより上へ書式付き文字列を書き込みます。
     *  @param[in]      screen   固定プロンプト ハンドルです。
     *  @param[in]      channel  書き込み先の標準ストリームです。
     *  @param[in]      fmt      printf 形式の書式文字列です。NULL の場合は空文字列として扱います。
     *  @param[in]      ...      @p fmt に対応する書式引数です。
     *  @return         成功時は対象ストリームへ書き込んだバイト数を返します。引数不正、書式処理失敗、
     *                  またはメモリ確保失敗の場合は -1 を返します。
     *  @note           ANSI CSI SGR エスケープ シーケンスは、色指定としてそのまま出力します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_pinned_prompt_printf(com_util_pinned_prompt *screen,
                                                                   com_util_pinned_prompt_channel_t channel,
                                                                   const char *fmt, ...)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 3, 4)))
#endif /* COMPILER_GCC */
        ;

    /**
     *  @brief          指定位置のステータス領域を有効または無効にします。
     *  @param[in]      screen    固定プロンプト ハンドルです。
     *  @param[in]      position  上部または下部のステータス領域を指定します。
     *  @param[in]      enable    0 以外の場合は有効にし、0 の場合は無効にします。
     *  @retval         COM_UTIL_OK                    ステータス領域の有効状態を変更しました。
     *  @retval         COM_UTIL_ERR_INVALID_ARGUMENT  @p screen が NULL、または @p position が不正です。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部のミューテックスで保護されており、同一 @p screen に対して複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_pinned_prompt_status_enable(
        com_util_pinned_prompt *screen, com_util_pinned_prompt_status_position_t position, int enable);

    /**
     *  @brief          指定位置のステータス領域へ表示内容を設定します。
     *  @param[in]      screen    固定プロンプト ハンドルです。
     *  @param[in]      position  上部または下部のステータス領域を指定します。
     *  @param[in]      align     左寄せまたは右寄せを指定します。
     *  @param[in]      content   表示する文字列です。NULL の場合は指定位置の内容を消去します。
     *  @retval         COM_UTIL_OK                    表示内容を設定しました。
     *  @retval         COM_UTIL_ERR_INVALID_ARGUMENT  @p screen が NULL、または位置や配置の指定が不正です。
     *  @retval         COM_UTIL_ERR_OUT_OF_MEMORY     表示内容を保持するメモリを確保できません。
     *  @note           ANSI CSI SGR エスケープ シーケンスは色指定としてそのまま出力し、表示幅を
     *                  0 として配置を計算します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部のミューテックスで保護されており、同一 @p screen に対して複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API
    com_util_pinned_prompt_status_set(com_util_pinned_prompt *screen, com_util_pinned_prompt_status_position_t position,
                                      com_util_pinned_prompt_status_align_t align, const char *content);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_PINNED_PROMPT_H */
