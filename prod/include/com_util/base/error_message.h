/**
 *******************************************************************************
 *  @file           error_message.h
 *  @brief          結果コードと OS エラー値を人間可読の文字列へ変換する API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/07/30
 *  @version        1.0.0
 *
 *  ログやユーザー向けメッセージへエラーを表示するための文字列化を提供します。\n
 *  共通結果コード (@ref COM_UTIL_OK 、`COM_UTIL_ERR_*`) と、OS 由来のエラー値
 *  (`errno`、Windows の `GetLastError()`) の双方を扱います。
 *
 *  OS エラー値の文字列化は、errno と Win32 エラー コードで API を分けています。\n
 *  同じ `int` で両方を受け取ると、値の由来を取り違えて誤ったメッセージを表示するため、
 *  ドメインを型と関数名で区別します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_BASE_ERROR_MESSAGE_H
#define COM_UTIL_BASE_ERROR_MESSAGE_H

#include <com_util/base/platform.h>
#include <com_util/base/result.h>
#include <com_util/com_util_export.h>
#include <stddef.h>

/**
 *  @ingroup        COM_UTIL_BASE
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          共通結果コードの名称を返します。
     *  @param[in]      result  共通結果コード (@ref COM_UTIL_OK または `COM_UTIL_ERR_*`)。
     *  @return         結果コードに対応する静的文字列を返します。NULL は返しません。
     *
     *  返す文字列はコード名に対応する英小文字の短い説明です
     *  (例: @ref COM_UTIL_ERR_INVALID_ARGUMENT なら "invalid argument")。\n
     *  未知の値には "unknown result code" を返します。
     *
     *  返却する文字列は静的領域にあり、呼び出し側で解放してはなりません。\n
     *  書式文字列としてではなく、`%s` の引数として使用してください。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持たず、静的文字列を返すだけです。
     */
    COM_UTIL_EXPORT const char *COM_UTIL_API com_util_result_to_string(int result);

    /**
     *  @brief          errno の値に対応するメッセージを取得します。
     *  @param[out]     buf           メッセージの格納先。NULL を渡してはなりません。\n
     *                                常に NUL 終端します。
     *  @param[in]      buf_size      @p buf のバイト数。1 以上を指定してください。
     *  @param[in]      errno_value   errno の値。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。\n
     *                  メッセージが収まらない場合も切り詰めて格納し @ref COM_UTIL_OK を返します。
     *
     *  Linux では `strerror_r`、Windows では `strerror_s` を使用します。\n
     *  スレッド セーフでない `strerror` は使用しません。
     *
     *  @attention      Windows の `GetLastError()` の値を渡してはなりません。
     *                  errno と Win32 エラー コードは別の体系であり、
     *                  Win32 の値には com_util_win32_error_message() を使用します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  呼び出し側のバッファーへ書き込み、共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_errno_message(char *buf, size_t buf_size, int errno_value);

#if defined(PLATFORM_WINDOWS)

    /**
     *  @brief          Win32 エラー コードに対応するメッセージを取得します (Windows 専用)。
     *  @param[out]     buf         メッセージの格納先。NULL を渡してはなりません。\n
     *                              常に NUL 終端します。
     *  @param[in]      buf_size    @p buf のバイト数。1 以上を指定してください。
     *  @param[in]      error_code  `GetLastError()` が返す Win32 エラー コード。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。\n
     *                  メッセージが収まらない場合も切り詰めて格納し @ref COM_UTIL_OK を返します。
     *
     *  `FormatMessageW` で取得した文字列を UTF-8 へ変換して格納します。\n
     *  メッセージ末尾の改行は取り除きます。
     *
     *  @attention      errno の値を渡してはなりません。errno には
     *                  com_util_errno_message() を使用します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  呼び出し側のバッファーへ書き込み、共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_win32_error_message(char *buf, size_t buf_size, unsigned long error_code);

#endif /* PLATFORM_WINDOWS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_BASE_ERROR_MESSAGE_H */
