/**
 *******************************************************************************
 *  @file           stdlib.h
 *  @brief          stdlib 系 CRT 抽象 API。
 *  @author         Tetsuo Honda
 *  @date           2026/05/01
 *
 *  C 標準ユーティリティ関数をプラットフォーム差異なしで使用できるラッパーを提供します。\n
 *  Windows では MSVC が非推奨とする関数の代替安全版 (_dupenv_s 等) を使用します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
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
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          環境変数の値を取得する。
     *
     *  指定された環境変数が設定されている場合、その値を @p buf に格納して 0 を返す。\n
     *  @p buf に NULL を渡した場合は存在確認のみ行い、値のコピーを省略する。\n
     *  Windows では @c _dupenv_s を使用して MSVC セキュリティ警告を回避する。
     *
     *  @param[in]      name        環境変数名 (null 終端文字列)。
     *  @param[out]     buf         値の格納先。NULL を指定すると存在確認のみ行う。
     *  @param[in]      buf_size    @p buf のバイト数。@p buf が NULL の場合は無視。
     *  @return         0:         変数が設定されており、@p buf が NULL でない場合は値をコピー済み。
     *  @return         -1:        変数が未設定。
     *  @return         ERANGE:    @p buf が NULL でなく、バッファが値を格納するのに不足。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  環境変数の読み取りのみを行います。他スレッドが同時に環境変数を変更する場合は、呼び出し側で同期してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_getenv(const char *name, char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_CRT_STDLIB_H */
