/**
 *******************************************************************************
 *  @file           time.h
 *  @brief          time 系 CRT 抽象 API。
 *  @author         Tetsuo Honda
 *  @date           2026/04/22
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef COM_UTIL_CRT_TIME_H
#define COM_UTIL_CRT_TIME_H

#include <time.h>
#include <com_util_export.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          `gmtime_r` / `gmtime_s` のラッパー。UTC 時刻をスレッドセーフに変換します。
     *  @param[out]     utc_tm  変換結果の格納先。NULL を渡してはなりません。
     *  @param[in]      timep   変換するエポック秒へのポインタ。
     *  @return         成功時は 0、失敗時は -1 を返します。
     *  @warning        @p timep が NULL の場合は失敗します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_gmtime(struct tm *utc_tm,
                                                      const time_t *timep);

    /**
     *  @brief          `localtime_r` / `localtime_s` のラッパー。ローカル時刻をスレッドセーフに変換します。
     *  @param[out]     local_tm  変換結果の格納先。NULL を渡してはなりません。
     *  @param[in]      timep     変換するエポック秒へのポインタ。
     *  @return         成功時は 0、失敗時は -1 を返します。
     *  @warning        @p timep が NULL の場合は失敗します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_localtime(struct tm *local_tm,
                                                         const time_t *timep);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COM_UTIL_CRT_TIME_H */
