/**
 *******************************************************************************
 *  @file           prompt.h
 *  @brief          汎用プロンプトヘルパー API。
 *  @author         Tetsuo Honda
 *  @date           2026/04/30
 *  @version        1.0.0
 *
 *  @details
 *  対話的な1行入力を提供します。\n
 *  TTY（対話端末）では以下のキー操作が有効です：
 *  - 上/下矢印キー : 入力履歴を遡る/進む
 *  - 左/右矢印キー : カーソル移動
 *  - Home / End    : 行頭/行末へ移動
 *  - BackSpace     : カーソル前の文字を削除
 *  - Delete        : カーソル上の文字を削除
 *  - Ctrl+C        : 入力中断（0 を返す）
 *  - Enter         : 確定
 *
 *  TTY でない場合（パイプ・リダイレクト等）は fgets() にフォールバックします。\n
 *
 *  @par 使用例（固定プロンプト）
 *  @code{.c}
    #include <com_util/prompt/prompt.h>

    int main(void) {
        char buf[256];
        com_util_prompt_t *prompt = com_util_prompt_create(0);
        while (com_util_prompt_readline(prompt, buf, sizeof(buf), ">> ")) {
            printf("入力: %s\n", buf);
        }
        com_util_prompt_dispose(prompt);
        return 0;
    }
 *  @endcode
 *
 *  @par 使用例（フォーマットプロンプト）
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
 *******************************************************************************
 */

#ifndef COM_UTIL_PROMPT_H
#define COM_UTIL_PROMPT_H

#include <stddef.h>
#include <com_util/base/platform.h>
#include <com_util/base/compiler.h>
#include <com_util_export.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 *  @brief  各コンテキストで保持する履歴エントリ数のデフォルト値。
 */
#define COM_UTIL_PROMPT_HISTORY_DEFAULT 64

/**
 *  @brief  1 行の最大バイト数（NUL 終端含む）。
 */
#define COM_UTIL_PROMPT_LINE_MAX 4096

/**
 *  @brief  プロンプトハンドルの不透明型。
 */
typedef struct com_util_prompt_t com_util_prompt_t;

/**
 *  @brief      プロンプトハンドルを生成する。
 *  @param[in]  history_max  各コンテキストの履歴エントリ数上限。
 *                           0 を指定すると @c COM_UTIL_PROMPT_HISTORY_DEFAULT を使用する。
 *  @return     成功時は非 NULL ハンドル、失敗時は NULL。
 */
COM_UTIL_EXPORT com_util_prompt_t *COM_UTIL_API
com_util_prompt_create(size_t history_max);

/**
 *  @brief      プロンプトハンドルを解放する。
 *  @param[in]  prompt  com_util_prompt_create() が返したハンドル。NULL 可。
 *  @details    raw モード中の場合はターミナル設定を復元してから解放する。
 */
COM_UTIL_EXPORT void COM_UTIL_API
com_util_prompt_dispose(com_util_prompt_t *prompt);

/**
 *  @brief      固定プロンプト文字列を表示して 1 行入力を受け取る。
 *  @param[in]  p           ハンドル。
 *  @param[out] buf         入力結果を格納するバッファ（末尾の改行は含まない）。
 *  @param[in]  buf_size    バッファのサイズ（バイト）。
 *  @param[in]  prompt_str  表示するプロンプト文字列。NULL の場合は "" として扱う。
 *  @return     入力確定時は 1、EOF / Ctrl+C 時は 0。
 */
#define com_util_prompt_readline(p, buf, buf_size, prompt_str) \
    _com_util_prompt_readline((p), (buf), (buf_size), (prompt_str), __FILE__, __LINE__)

/**
 *  @brief      printf スタイルのフォーマットでプロンプトを表示して 1 行入力を受け取る。
 *  @param[in]  p        ハンドル。
 *  @param[out] buf      入力結果を格納するバッファ（末尾の改行は含まない）。
 *  @param[in]  buf_size バッファのサイズ（バイト）。
 *  @param[in]  fmt      printf 形式のフォーマット文字列。NULL の場合は "" として扱う。
 *  @param[in]  ...      フォーマット引数。
 *  @return     入力確定時は 1、EOF / Ctrl+C 時は 0。
 *  @details    プロンプト文字列バッファはハンドル内に保持し、必要に応じて自動拡張します。
 */
#define com_util_prompt_readline_fmt(p, buf, buf_size, fmt, ...) \
    _com_util_prompt_readline_fmt((p), (buf), (buf_size), __FILE__, __LINE__, (fmt), ##__VA_ARGS__)

/* ---- 内部実装関数（直接呼び出し不要） ---- */

/**
 *  @brief  com_util_prompt_readline() の内部実装関数。
 */
COM_UTIL_EXPORT int COM_UTIL_API
_com_util_prompt_readline(com_util_prompt_t *prompt,
                           char              *buf,
                           size_t             buf_size,
                           const char        *prompt_str,
                           const char        *file,
                           int                line);

/**
 *  @brief  com_util_prompt_readline_fmt() の内部実装関数。
 */
COM_UTIL_EXPORT int COM_UTIL_API
_com_util_prompt_readline_fmt(com_util_prompt_t *p,
                               char              *buf,
                               size_t             buf_size,
                               const char        *file,
                               int                line,
                               const char        *fmt,
                               ...)
#if defined(COMPILER_GCC)
    __attribute__((format(printf, 6, 7)))
#endif /* COMPILER_GCC */
    ;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COM_UTIL_PROMPT_H */
