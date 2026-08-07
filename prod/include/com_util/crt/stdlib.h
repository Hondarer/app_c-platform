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
#include <stdint.h>
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
     *  @param[out]     detail_out  エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *                  NULL 以外を指定した場合、成功時は空の値を格納します。
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
     *  @param[out]     detail_out エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *                  NULL 以外を指定した場合、成功時は空の値を格納します。
     *  @return         成功時は @ref COM_UTIL_OK 、失敗時は共通結果コードを返します。
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
     *  @param[out]     detail_out エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *                  NULL 以外を指定した場合、成功時は空の値を格納します。
     *  @return         成功時は @ref COM_UTIL_OK 、失敗時は共通結果コードを返します。\n
     *                  変数が設定されていない場合も成功として扱います。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  環境変数の変更は、他スレッドによる読み取りと競合します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_unsetenv(const char *name, com_util_error *detail_out);

    /**
     *  @brief          文字列を 64 bit 符号付き整数へ変換します (`strtoll` の安全版)。
     *
     *  `strtoll` と異なり、変換位置 (`endptr`) を返しません。\n
     *  文字列全体が整数として解釈されたことを関数側で検査し、末尾に余分な文字が残る場合は失敗とします。\n
     *  先頭の空白と符号は許容します。空文字列と、空白のみの文字列は失敗とします。
     *
     *  @param[out]     value_out  変換結果の格納先。NULL を渡してはなりません。\n
     *                             失敗時の内容は不定です。
     *  @param[in]      text       変換する文字列 (null 終端)。NULL を渡してはなりません。
     *  @param[in]      base       基数。2 から 36、または 0 (接頭辞による自動判別) を指定します。
     *  @return         成功時は @ref COM_UTIL_OK を返します。
     *  @return         @p value_out もしくは @p text が NULL の場合、または @p base が範囲外の場合は
     *                  @ref COM_UTIL_ERR_INVALID_ARGUMENT を返します。
     *  @return         整数として解釈できない場合、または末尾に余分な文字が残る場合は
     *                  @ref COM_UTIL_ERR_INVALID_INTEGER を返します。
     *  @return         `int64_t` で表現できない値の場合は @ref COM_UTIL_ERR_OUT_OF_RANGE を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_parse_int64(int64_t *value_out, const char *text, int base);

    /**
     *  @brief          文字列を 64 bit 符号なし整数へ変換します (`strtoull` の安全版)。
     *
     *  検査の方針は @ref com_util_parse_int64 と同じです。\n
     *  加えて、符号 `'-'` で始まる文字列を @ref COM_UTIL_ERR_OUT_OF_RANGE として拒否します。
     *
     *  @param[out]     value_out  変換結果の格納先。NULL を渡してはなりません。\n
     *                             失敗時の内容は不定です。
     *  @param[in]      text       変換する文字列 (null 終端)。NULL を渡してはなりません。
     *  @param[in]      base       基数。2 から 36、または 0 (接頭辞による自動判別) を指定します。
     *  @return         成功時は @ref COM_UTIL_OK を返します。
     *  @return         @p value_out もしくは @p text が NULL の場合、または @p base が範囲外の場合は
     *                  @ref COM_UTIL_ERR_INVALID_ARGUMENT を返します。
     *  @return         整数として解釈できない場合、または末尾に余分な文字が残る場合は
     *                  @ref COM_UTIL_ERR_INVALID_INTEGER を返します。
     *  @return         負値が指定された場合、または `uint64_t` で表現できない値の場合は
     *                  @ref COM_UTIL_ERR_OUT_OF_RANGE を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_parse_uint64(uint64_t *value_out, const char *text, int base);

    /**
     *  @brief          文字列を `int` へ変換します (`atoi` の安全版)。
     *
     *  @ref com_util_parse_int64 で解析したうえで、`int` の範囲に収まることを検査します。
     *
     *  @param[out]     value_out  変換結果の格納先。NULL を渡してはなりません。\n
     *                             失敗時の内容は不定です。
     *  @param[in]      text       変換する文字列 (null 終端)。NULL を渡してはなりません。
     *  @param[in]      base       基数。2 から 36、または 0 (接頭辞による自動判別) を指定します。
     *  @return         成功時は @ref COM_UTIL_OK を返します。
     *  @return         @p value_out もしくは @p text が NULL の場合、または @p base が範囲外の場合は
     *                  @ref COM_UTIL_ERR_INVALID_ARGUMENT を返します。
     *  @return         整数として解釈できない場合、または末尾に余分な文字が残る場合は
     *                  @ref COM_UTIL_ERR_INVALID_INTEGER を返します。
     *  @return         `int` で表現できない値の場合は @ref COM_UTIL_ERR_OUT_OF_RANGE を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_parse_int(int *value_out, const char *text, int base);

    /**
     *  @brief          文字列を `double` へ変換します (`atof` の安全版)。
     *
     *  検査の方針は @ref com_util_parse_int64 と同じで、文字列全体が数値として解釈されたことを検査します。\n
     *  `strtod` が受け付ける表記 (指数表記、16 進浮動小数、`inf`、`nan`) をそのまま受け付けます。
     *
     *  @param[out]     value_out  変換結果の格納先。NULL を渡してはなりません。\n
     *                             失敗時の内容は不定です。
     *  @param[in]      text       変換する文字列 (null 終端)。NULL を渡してはなりません。
     *  @return         成功時は @ref COM_UTIL_OK を返します。
     *  @return         @p value_out もしくは @p text が NULL の場合は
     *                  @ref COM_UTIL_ERR_INVALID_ARGUMENT を返します。
     *  @return         数値として解釈できない場合、または末尾に余分な文字が残る場合は
     *                  @ref COM_UTIL_ERR_INVALID_INTEGER を返します。
     *  @return         `double` で表現できない値の場合は @ref COM_UTIL_ERR_OUT_OF_RANGE を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_parse_double(double *value_out, const char *text);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_CRT_STDLIB_H */
