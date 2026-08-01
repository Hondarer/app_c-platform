/**
 *******************************************************************************
 *  @file           stdlib.h
 *  @brief          stdlib 系の CRT 関数を抽象化する API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/05/01
 *
 *  C 標準ユーティリティ関数をプラットフォーム差異なしで使用できるラッパーを提供します。\n
 *  Windows では MSVC が非推奨とする関数の代替安全版 (_dupenv_s 等) を使用します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_CRT_STDLIB_H
#define COM_UTIL_CRT_STDLIB_H

#include <stddef.h>
#include <com_util/base/error.h>
#include <com_util/base/result.h>
#include <com_util/com_util_export.h>

/**
 *  @ingroup        COM_UTIL_CRT
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          環境変数の値を取得します。
     *
     *  指定された環境変数が設定されている場合、その値を @p buf に格納する。\n
     *  @p buf に NULL を渡した場合は存在確認のみ行い、値のコピーを省略する。\n
     *  Windows では @c _dupenv_s を使用して MSVC セキュリティ警告を回避します。
     *
     *  @param[in]      name        環境変数名 (null 終端文字列)。NULL を渡してはなりません。
     *  @param[out]     buf         値の格納先です。NULL を指定すると存在確認のみ行います。\n
     *                              変数が設定されていない場合は空文字列を格納します。
     *  @param[in]      buf_size    @p buf のバイト数。@p buf が NULL の場合は無視。
     *  @param[out]     exists_out  変数が設定されている場合は 1、設定されていない場合は 0 を格納します。\n
     *                              NULL も指定できます。戻り値が @ref COM_UTIL_OK または
     *                              @ref COM_UTIL_ERR_BUFFER_TOO_SMALL の場合に有効です。
     *  @param[out]     detail_out  エラー詳細の格納先。NULL 可。成功時は空の値を格納します。
     *  @return         成功時は @ref COM_UTIL_OK を返します。変数の設定有無は @p exists_out で確認します。
     *  @return         @p name が NULL の場合は @ref COM_UTIL_ERR_INVALID_ARGUMENT を返します。
     *  @return         値の格納先が不足している場合は @ref COM_UTIL_ERR_BUFFER_TOO_SMALL を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  環境変数の読み取りのみを行います。他スレッドが同時に環境変数を変更する場合は、呼び出し側で同期してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_getenv(const char *name, char *buf, size_t buf_size, int *exists_out,
                                                     com_util_error *detail_out);

    /**
     *  @brief          環境変数の値を設定します。
     *
     *  Linux では @c setenv、Windows では @c _putenv_s を使用します。\n
     *  設定は呼び出し元プロセスにのみ反映され、親プロセスへは伝わりません。
     *
     *  @param[in]      name       環境変数名 (null 終端文字列)。NULL、空文字列、
     *                             `'='` を含む文字列を渡してはなりません。
     *  @param[in]      value      設定する値 (null 終端文字列)。NULL を渡してはなりません。
     *  @param[in]      overwrite  変数がすでに設定されている場合に上書きするかどうか。\n
     *                             0 のとき既存の値を保持します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。成功時は空の値を格納します。
     *  @return         成功時は @ref COM_UTIL_OK、失敗時は共通結果コードを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  環境変数の変更は、他スレッドによる読み取りと競合します。
     *  マルチスレッド化の前に設定を完了させるか、呼び出し側で同期してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_setenv(const char *name, const char *value, int overwrite,
                                                     com_util_error *detail_out);

    /**
     *  @brief          環境変数を削除します。
     *
     *  Linux では @c unsetenv、Windows では値に空文字列を指定した @c _putenv_s を使用します。\n
     *  Windows は空文字列の設定を削除として扱うため、値が空の環境変数を作ることはできません。
     *
     *  @param[in]      name  環境変数名 (null 終端文字列)。NULL、空文字列、
     *                        `'='` を含む文字列を渡してはなりません。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。成功時は空の値を格納します。
     *  @return         成功時は @ref COM_UTIL_OK、失敗時は共通結果コードを返します。\n
     *                  変数が設定されていない場合も成功として扱います。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  環境変数の変更は、他スレッドによる読み取りと競合します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_unsetenv(const char *name, com_util_error *detail_out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_CRT_STDLIB_H */
