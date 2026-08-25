/**
 *******************************************************************************
 *  @file           filetime_conv.h
 *  @brief          Windows の FILETIME と com_util_timespec を相互変換する内部 API を提供します。
 *
 *  FILETIME は 1601-01-01T00:00:00Z を起点とする 100 ナノ秒単位の値です。
 *  Unix epoch (1970-01-01T00:00:00Z) を起点とする com_util_timespec との間で、
 *  起点の差と単位の差を吸収します。
 *
 *  変換は Windows でのみ必要なため、本ヘッダーは PLATFORM_WINDOWS でのみ内容を持ちます。
 *  実体を持つ翻訳単位を増やさないよう、変換は static inline 関数として提供します。
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは複数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_CLOCK_FILETIME_CONV_INTERNAL_H
#define COM_UTIL_CLOCK_FILETIME_CONV_INTERNAL_H

#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)

    #include <com_util/base/compiler.h>
    #include <com_util/base/windows_sdk.h>
    #include <com_util/clock/timespec.h>
    #include <stdint.h>

    #define COM_UTIL_FILETIME_UNITS_PER_SEC    (10000000ULL)   /**< FILETIME 単位 (100ns) / 秒。 */
    #define COM_UTIL_NSEC_PER_FILETIME_UNIT    (100ULL)        /**< ナノ秒 / FILETIME 単位。 */
    #define COM_UTIL_FILETIME_EPOCH_OFFSET_SEC (11644473600LL) /**< 1601-01-01 → 1970-01-01 の差 (秒)。 */

/**
 *  @brief          FILETIME を com_util_timespec へ変換します。
 *  @param[in]      filetime  変換元。NULL を渡してはなりません。
 *  @param[out]     ts        変換結果の格納先。NULL を渡してはなりません。
 *
 *  1601-01-01 起点の 100 ナノ秒単位を、Unix epoch 起点の秒とナノ秒へ分解します。
 */
static inline void com_util_internal_filetime_to_timespec(const FILETIME *filetime, com_util_timespec *ts)
{
    ULARGE_INTEGER uli;

    uli.LowPart = filetime->dwLowDateTime;
    uli.HighPart = filetime->dwHighDateTime;

    ts->tv_sec = (time_t)(uli.QuadPart / COM_UTIL_FILETIME_UNITS_PER_SEC) - COM_UTIL_FILETIME_EPOCH_OFFSET_SEC;
    ts->tv_nsec = (int64_t)((uli.QuadPart % COM_UTIL_FILETIME_UNITS_PER_SEC) * COM_UTIL_NSEC_PER_FILETIME_UNIT);
}

/**
 *  @brief          com_util_timespec を FILETIME へ変換します。
 *  @param[in]      ts            変換元。NULL を渡してはなりません。
 *  @param[out]     filetime_out  変換結果の格納先。NULL を渡してはなりません。
 *
 *  ナノ秒部は 100 ナノ秒単位へ切り捨てます。FILETIME の分解能が 100 ナノ秒であるためです。
 *
 *  @attention      Unix epoch より前の時刻のうち、FILETIME の起点 (1601-01-01) より前になる値は
 *                  表現できません。この場合は 0 を格納します。
 */
static inline void com_util_internal_timespec_to_filetime(const com_util_timespec *ts, FILETIME *filetime_out)
{
    const int64_t filetime_sec = (int64_t)ts->tv_sec + COM_UTIL_FILETIME_EPOCH_OFFSET_SEC;
    ULARGE_INTEGER uli;

    if (filetime_sec < 0)
    {
        filetime_out->dwLowDateTime = 0;
        filetime_out->dwHighDateTime = 0;
        return;
    }

    uli.QuadPart = (ULONGLONG)filetime_sec * COM_UTIL_FILETIME_UNITS_PER_SEC;
    uli.QuadPart += (ULONGLONG)(ts->tv_nsec / (int64_t)COM_UTIL_NSEC_PER_FILETIME_UNIT);

    filetime_out->dwLowDateTime = uli.LowPart;
    filetime_out->dwHighDateTime = uli.HighPart;
}

#endif /* PLATFORM_WINDOWS */

#endif /* COM_UTIL_CLOCK_FILETIME_CONV_INTERNAL_H */
