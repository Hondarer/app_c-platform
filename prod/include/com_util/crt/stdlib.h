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
     *  @param[in]      name        環境変数名 (null 終端文字列)。NULL を渡した場合は EINVAL を返します。
     *  @param[out]     buf         値の格納先です。NULL を指定すると存在確認のみ行います。\n
     *                              変数が設定されていない場合は空文字列を格納します。
     *  @param[in]      buf_size    @p buf のバイト数。@p buf が NULL の場合は無視。
     *  @param[out]     exists_out  変数が設定されている場合は 1、設定されていない場合は 0 を格納します。\n
     *                              NULL も指定できます。戻り値が 0 または ERANGE の場合に有効です。
     *  @return         成功時は 0 を返します。変数の設定有無は @p exists_out で確認します。
     *  @return         @p name が NULL の場合は EINVAL を返します。
     *  @return         変数が設定されており、@p buf が NULL でなく、値を格納するには不足している場合は ERANGE を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  環境変数の読み取りのみを行います。他スレッドが同時に環境変数を変更する場合は、呼び出し側で同期してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_getenv(const char *name, char *buf, size_t buf_size, int *exists_out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_CRT_STDLIB_H */
