/**
 *******************************************************************************
 *  @file           prompt.h
 *  @brief          対話的な 1 行入力を提供する汎用プロンプト API です。
 *  @author         Tetsuo Honda
 *  @date           2026/04/30
 *  @version        1.0.0
 *
 *  対話的な 1 行入力を提供します。\n
 *  TTY (対話端末) では以下のキー操作を使用できます。
 *  - 上/下矢印キー : 入力履歴を遡る/進む
 *  - 左/右矢印キー : カーソル移動
 *  - Home / End    : 行頭/行末へ移動
 *  - BackSpace     : カーソル前の文字を削除
 *  - Delete        : カーソル上の文字を削除
 *  - Ctrl+C        : 入力中断 (0 を返す)
 *  - Enter         : 確定
 *
 *  TTY でない場合 (パイプ・リダイレクト等) は fgets() にフォールバックします。\n
 *
 *  @par 使用例 (固定プロンプト)
 *  @code{.c}
    #include <com_util/prompt/prompt.h>

    int main(void) {
        char buf[256];
        com_util_prompt *prompt = com_util_prompt_create(NULL);
        while (com_util_prompt_readline(prompt, buf, sizeof(buf), ">> ")) {
            printf("入力: %s\n", buf);
        }
        com_util_prompt_dispose(prompt);
        return 0;
    }
 *  @endcode
 *
 *  @par 使用例 (フォーマット プロンプト)
 *  @code{.c}
    while (com_util_prompt_readline_fmt(prompt, buf, sizeof(buf),
                                        "[%s]> ", state_name)) {
        // ...
    }
 *  @endcode
 *
 *  @par 複数箇所からの使用
 *  同一ハンドルを複数箇所で呼び出すと、呼び出し元のファイル名・行番号ごとに
 *  独立した履歴が自動的に割り当てられます。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_PROMPT_H
#define COM_UTIL_PROMPT_H

#include <stddef.h>
#include <com_util/base/platform.h>
#include <com_util/base/compiler.h>
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
 *  @brief  各コンテキストで保持する履歴エントリ数の既定値です。
 */
#define COM_UTIL_PROMPT_HISTORY_DEFAULT 64

/**
 *  @brief  NUL 終端を含む入力バッファーの既定最大バイト数です。
 */
#define COM_UTIL_PROMPT_INPUT_BYTES_DEFAULT 4096

    /**
     *  @brief  プロンプトを操作する不透明ハンドルです。
     */
    typedef struct com_util_prompt com_util_prompt;

    /**
     *  @brief  プロンプトの生成オプションです。
     */
    typedef struct com_util_prompt_options
    {
        /**
         *  @brief  将来拡張用のフラグです。現時点では 0 を指定してください。
         */
        unsigned int flags;

        /**
         *  @brief  構造体配置用の予約領域です。現時点では 0 を指定してください。
         */
        unsigned int reserved;

        /**
         *  @brief  各コンテキストで保持する履歴エントリ数の上限です。
         *          0 の場合は @c COM_UTIL_PROMPT_HISTORY_DEFAULT を使用します。
         */
        size_t history_max;

        /**
         *  @brief  NUL 終端を含む入力編集バッファーの初期バイト数です。
         *          0 の場合は実装の既定値を使用します。
         */
        size_t input_initial_capacity;

        /**
         *  @brief  NUL 終端を含む入力編集バッファーの最大バイト数です。
         *          0 の場合は @c COM_UTIL_PROMPT_INPUT_BYTES_DEFAULT を使用します。
         */
        size_t input_max_bytes;
    } com_util_prompt_options;

    /**
     *  @brief          プロンプト ハンドルを生成します。
     *  @param[in]      options  生成オプションです。NULL の場合は既定設定を使用します。
     *  @return         成功時は生成したハンドルを返します。メモリを確保できない場合は NULL を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。各呼び出しは独立したハンドルを生成します。
     */
    COM_UTIL_EXPORT com_util_prompt *COM_UTIL_API com_util_prompt_create(const com_util_prompt_options *options);

    /**
     *  @brief          プロンプト ハンドルを解放します。
     *  @param[in]      prompt  com_util_prompt_create() が返したハンドルです。NULL も指定できます。
     *
     *  raw モード中の場合は、端末設定を復元してから解放します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  解放対象の @p prompt を他スレッドが使用していないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_prompt_dispose(com_util_prompt *prompt);

/**
 *  @brief          固定プロンプト文字列を表示して 1 行入力を受け取ります。
 *  @param[in]      p           プロンプト ハンドルです。
 *  @param[out]     buf         入力結果を格納するバッファーです。終端の改行は格納しません。
 *  @param[in]      buf_size    @p buf のバイト数です。
 *  @param[in]      prompt_str  表示するプロンプト文字列です。NULL の場合は空文字列として扱います。
 *  @return         入力を確定した場合は 1 を返します。EOF、Ctrl+C、引数不正、または内部エラーの場合は
 *                  0 を返します。
 */
#define com_util_prompt_readline(p, buf, buf_size, prompt_str) \
    com_util_prompt_readline_at((p), (buf), (buf_size), (prompt_str), __FILE__, __LINE__)

/**
 *  @brief          printf 形式でプロンプトを生成して 1 行入力を受け取ります。
 *  @param[in]      p         プロンプト ハンドルです。
 *  @param[out]     buf       入力結果を格納するバッファーです。終端の改行は格納しません。
 *  @param[in]      buf_size  @p buf のバイト数です。
 *  @param[in]      fmt       printf 形式の書式文字列です。NULL の場合は空文字列として扱います。
 *  @param[in]      ...       @p fmt に対応する書式引数です。
 *  @return         入力を確定した場合は 1 を返します。EOF、Ctrl+C、引数不正、または内部エラーの場合は
 *                  0 を返します。
 *
 *  プロンプト文字列バッファーはハンドル内に保持し、必要に応じて自動拡張します。
 */
#define com_util_prompt_readline_fmt(p, buf, buf_size, fmt, ...) \
    com_util_prompt_readline_fmt_at((p), (buf), (buf_size), __FILE__, __LINE__, (fmt), ##__VA_ARGS__)

    /**
     *  @brief          呼び出し元を明示して 1 行入力を受け取ります。
     *
     *  通常は com_util_prompt_readline() を使用してください。
     *
     *  @param[in]      prompt      プロンプト ハンドルです。
     *  @param[out]     buf         入力結果を格納するバッファーです。
     *  @param[in]      buf_size    @p buf のバイト数です。
     *  @param[in]      prompt_str  表示するプロンプト文字列です。NULL の場合は空文字列として扱います。
     *  @param[in]      file        履歴を識別する呼び出し元ファイル名です。
     *  @param[in]      line        履歴を識別する呼び出し元行番号です。
     *  @return         入力を確定した場合は 1、それ以外の場合は 0 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  同一 @p prompt への並行呼び出しは未定義動作です。入力は 1 スレッドから行ってください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_prompt_readline_at(com_util_prompt *prompt, char *buf, size_t buf_size,
                                                                 const char *prompt_str, const char *file, int line);

    /**
     *  @brief          呼び出し元を明示し、printf 形式のプロンプトで 1 行入力を受け取ります。
     *
     *  通常は com_util_prompt_readline_fmt() を使用してください。
     *
     *  @param[in]      p         プロンプト ハンドルです。
     *  @param[out]     buf       入力結果を格納するバッファーです。
     *  @param[in]      buf_size  @p buf のバイト数です。
     *  @param[in]      file      履歴を識別する呼び出し元ファイル名です。
     *  @param[in]      line      履歴を識別する呼び出し元行番号です。
     *  @param[in]      fmt       printf 形式の書式文字列です。NULL の場合は空文字列として扱います。
     *  @param[in]      ...       @p fmt に対応する書式引数です。
     *  @return         入力を確定した場合は 1、それ以外の場合は 0 を返します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_prompt_readline_fmt_at(com_util_prompt *p, char *buf, size_t buf_size,
                                                                     const char *file, int line, const char *fmt, ...)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 6, 7)))
#endif /* COMPILER_GCC */
        ;

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_PROMPT_H */
