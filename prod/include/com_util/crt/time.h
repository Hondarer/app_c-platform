/**
 *******************************************************************************
 *  @file           time.h
 *  @brief          time 系の CRT 関数を抽象化する API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/04/22
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_CRT_TIME_H
#define COM_UTIL_CRT_TIME_H

#include <stddef.h>
#include <time.h>
#include <com_util/base/result.h>
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
     *  @brief          `gmtime_r` / `gmtime_s` のラッパー。UTC 時刻を変換します。
     *  @param[out]     utc_tm  変換結果の格納先。NULL を渡してはなりません。
     *  @param[in]      timep   変換するエポック秒へのポインター。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *  @warning        @p timep が NULL の場合は失敗します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  スレッド セーフな gmtime_r / gmtime_s を使用しており、内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_gmtime(struct tm *utc_tm, const time_t *timep);

    /**
     *  @brief          `localtime_r` / `localtime_s` のラッパー。ローカル時刻を変換します。
     *  @param[out]     local_tm  変換結果の格納先。NULL を渡してはなりません。
     *  @param[in]      timep     変換するエポック秒へのポインター。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *  @warning        @p timep が NULL の場合は失敗します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  スレッド セーフな localtime_r / localtime_s を使用しており、内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_localtime(struct tm *local_tm, const time_t *timep);

    /**
     *  @brief          `ctime_r` / `ctime_s` のラッパー。エポック秒をローカル時刻の文字列へ変換します。
     *  @param[out]     buf       変換結果の格納先。NULL を渡してはなりません。
     *  @param[in]      buf_size  @p buf のサイズ (バイト)。26 以上が必要です。
     *  @param[in]      timep     変換するエポック秒へのポインター。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  変換結果は `Wed Jul  8 21:49:08 2026\n` 形式 (改行と終端文字を含む) の
     *  ローカル時刻文字列です。\n
     *  失敗時は @p buf の全体をゼロ化します (@p buf が NULL の場合を除く)。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  スレッド セーフな ctime_r / ctime_s を使用しており、内部に共有状態を持ちません。
     *
     *  @warning        @p timep が NULL の場合、または @p buf_size が 26 未満の場合は失敗します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_ctime(char *buf, size_t buf_size, const time_t *timep);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_CRT_TIME_H */
