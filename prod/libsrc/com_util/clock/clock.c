/**
 *******************************************************************************
 *  @file           clock.c
 *  @brief          プラットフォーム抽象クロック取得ユーティリティー実装。
 *  @author         Tetsuo Honda
 *  @date           2026/04/19
 *  @version        1.0.0
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/base/platform.h>
#include <com_util/clock/clock.h>
#include <com_util/crt/time.h>
#include <stdio.h>
#include <string.h>

#if defined(PLATFORM_LINUX)
    #include <time.h>
#elif defined(PLATFORM_WINDOWS)
    #include <com_util/base/windows_sdk.h>
#endif /* PLATFORM_ */

/* 変換定数 */
#define MSEC_PER_SEC              (1000ULL)       /* ミリ秒 / 秒 */
#define NSEC_PER_SEC              (1000000000LL)  /* ナノ秒 / 秒 */
#define NSEC_PER_MSEC             (1000000ULL)    /* ナノ秒 / ミリ秒 */
#define SEC_PER_DAY               (86400LL)       /* 秒 / 日 */
#define FILETIME_UNITS_PER_SEC    (10000000ULL)   /* FILETIME 単位 (100ns) / 秒 */
#define NSEC_PER_FILETIME_UNIT    (100ULL)        /* ナノ秒 / FILETIME 単位 */
#define FILETIME_EPOCH_OFFSET_SEC (11644473600LL) /* 1601-01-01 → 1970-01-01 の差 (秒) */

static const char s_iso8601_local_fallback[] = "0000-00-00T00:00:00.000+00:00";
static const char s_iso8601_utc_fallback[] = "0000-00-00T00:00:00.000Z";

static void clock_fill_timespec(struct timespec *ts, int64_t tv_sec, int32_t tv_nsec)
{
    ts->tv_sec = (time_t)tv_sec;
    ts->tv_nsec = (long)tv_nsec;
}

static void clock_write_fallback(char *buf, size_t buf_size, const char *fallback)
{
    if (buf == NULL || buf_size == 0)
    {
        return;
    }

    snprintf(buf, buf_size, "%s", fallback);
}

static int64_t clock_days_from_civil(int year, unsigned month, unsigned day)
{
    int adjusted_year = year - (month <= 2);
    int era_base;
    int era;
    unsigned yoe;
    unsigned month_offset;
    unsigned doy;
    unsigned doe;

    if (adjusted_year >= 0)
    {
        era_base = adjusted_year;
    }
    else
    {
        era_base = adjusted_year - 399;
    }
    era = era_base / 400;

    yoe = (unsigned)(adjusted_year - era * 400);

    if (month > 2)
    {
        month_offset = (unsigned)-3;
    }
    else
    {
        month_offset = 9;
    }
    doy = (153 * (month + month_offset) + 2) / 5 + day - 1;

    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;

    return (int64_t)era * 146097 + (int64_t)doe - 719468;
}

static int clock_utc_offset_minutes(const struct tm *local_tm, const struct tm *utc_tm, int *offset_minutes)
{
    int64_t local_days;
    int64_t utc_days;
    int local_seconds;
    int utc_seconds;
    int64_t delta_seconds;

    if (local_tm == NULL || utc_tm == NULL || offset_minutes == NULL)
    {
        return -1;
    }

    local_days = clock_days_from_civil(local_tm->tm_year + 1900, (unsigned)local_tm->tm_mon + 1,
                                       (unsigned)local_tm->tm_mday);
    utc_days = clock_days_from_civil(utc_tm->tm_year + 1900, (unsigned)utc_tm->tm_mon + 1,
                                     (unsigned)utc_tm->tm_mday);
    local_seconds = local_tm->tm_hour * 3600 + local_tm->tm_min * 60 + local_tm->tm_sec;
    utc_seconds = utc_tm->tm_hour * 3600 + utc_tm->tm_min * 60 + utc_tm->tm_sec;
    delta_seconds = (local_days - utc_days) * SEC_PER_DAY + (local_seconds - utc_seconds);

    *offset_minutes = (int)(delta_seconds / 60);
    return 0;
}

static int clock_format_iso8601_utc_from_tm(char *buf, size_t buf_size, const struct tm *utc_tm, int32_t tv_nsec)
{
    if (buf == NULL || buf_size < (size_t)(COM_UTIL_CLOCK_ISO8601_UTC_MSEC_LEN + 1) || utc_tm == NULL)
    {
        return -1;
    }

#if defined(COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif /* COMPILER_GCC */
    snprintf(buf, buf_size, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", utc_tm->tm_year + 1900, utc_tm->tm_mon + 1,
             utc_tm->tm_mday, utc_tm->tm_hour, utc_tm->tm_min, utc_tm->tm_sec, (int)(tv_nsec / 1000000));
#if defined(COMPILER_GCC)
#pragma GCC diagnostic pop
#endif /* COMPILER_GCC */
    return 0;
}

static int clock_format_iso8601_local_from_tm(char *buf, size_t buf_size, const struct tm *local_tm,
                                              int32_t tv_nsec, int offset_minutes)
{
    char offset_sign;
    int offset_hours;
    int offset_mins;

    if (buf == NULL || buf_size < (size_t)(COM_UTIL_CLOCK_ISO8601_LOCAL_MSEC_LEN + 1) || local_tm == NULL)
    {
        return -1;
    }

    offset_sign = '+';
    if (offset_minutes < 0)
    {
        offset_sign = '-';
        offset_minutes = -offset_minutes;
    }
    offset_hours = offset_minutes / 60;
    offset_mins = offset_minutes % 60;

#if defined(COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif /* COMPILER_GCC */
    snprintf(buf, buf_size, "%04d-%02d-%02dT%02d:%02d:%02d.%03d%c%02d:%02d", local_tm->tm_year + 1900,
             local_tm->tm_mon + 1, local_tm->tm_mday, local_tm->tm_hour, local_tm->tm_min, local_tm->tm_sec,
             (int)(tv_nsec / 1000000), offset_sign, offset_hours, offset_mins);
#if defined(COMPILER_GCC)
#pragma GCC diagnostic pop
#endif /* COMPILER_GCC */
    return 0;
}

/* doxygen コメントは、ヘッダーに記載 */
uint64_t com_util_get_monotonic_ms(void)
{
#if defined(PLATFORM_LINUX)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * MSEC_PER_SEC + (uint64_t)ts.tv_nsec / NSEC_PER_MSEC;
#elif defined(PLATFORM_WINDOWS)
    return (uint64_t)GetTickCount64();
#endif /* PLATFORM_ */
}

/* doxygen コメントは、ヘッダーに記載 */
void com_util_get_monotonic(int64_t *tv_sec, int32_t *tv_nsec)
{
#if defined(PLATFORM_LINUX)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    *tv_sec = (int64_t)ts.tv_sec;
    *tv_nsec = (int32_t)ts.tv_nsec;
#elif defined(PLATFORM_WINDOWS)
    ULONGLONG ms = GetTickCount64();
    *tv_sec = (int64_t)(ms / MSEC_PER_SEC);
    *tv_nsec = (int32_t)((ms % MSEC_PER_SEC) * NSEC_PER_MSEC);
#endif /* PLATFORM_ */
}

/* doxygen コメントは、ヘッダーに記載 */
void com_util_get_realtime(int64_t *tv_sec, int32_t *tv_nsec)
{
#if defined(PLATFORM_LINUX)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    *tv_sec = (int64_t)ts.tv_sec;
    *tv_nsec = (int32_t)ts.tv_nsec;
#elif defined(PLATFORM_WINDOWS)
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    *tv_sec = (int64_t)(uli.QuadPart / FILETIME_UNITS_PER_SEC) - FILETIME_EPOCH_OFFSET_SEC;
    *tv_nsec = (int32_t)((uli.QuadPart % FILETIME_UNITS_PER_SEC) * NSEC_PER_FILETIME_UNIT);
#endif /* PLATFORM_ */
}

/* doxygen コメントは、ヘッダーに記載 */
void com_util_get_realtime_utc(struct tm *utc_tm, int32_t *tv_nsec)
{
    int64_t realtime_sec;

    com_util_get_realtime(&realtime_sec, tv_nsec);

    {
        time_t realtime_time = (time_t)realtime_sec;
        if (com_util_gmtime(utc_tm, &realtime_time) != 0)
        {
            memset(utc_tm, 0, sizeof(*utc_tm));
        }
    }
}

/* doxygen コメントは、ヘッダーに記載 */
COM_UTIL_EXPORT int COM_UTIL_API
com_util_format_realtime_iso8601_local(char *buf, size_t buf_size, int64_t tv_sec, int32_t tv_nsec)
{
    time_t realtime_time;
    struct tm local_tm;
    struct tm utc_tm;
    int offset_minutes;

    if (tv_nsec < 0 || tv_nsec >= (int32_t)NSEC_PER_SEC)
    {
        clock_write_fallback(buf, buf_size, s_iso8601_local_fallback);
        return -1;
    }

    realtime_time = (time_t)tv_sec;

    if (com_util_localtime(&local_tm, &realtime_time) != 0 ||
        com_util_gmtime(&utc_tm, &realtime_time) != 0 ||
        clock_utc_offset_minutes(&local_tm, &utc_tm, &offset_minutes) != 0 ||
        clock_format_iso8601_local_from_tm(buf, buf_size, &local_tm, tv_nsec, offset_minutes) != 0)
    {
        clock_write_fallback(buf, buf_size, s_iso8601_local_fallback);
        return -1;
    }

    return 0;
}

/* doxygen コメントは、ヘッダーに記載 */
COM_UTIL_EXPORT int COM_UTIL_API
com_util_format_realtime_iso8601_utc(char *buf, size_t buf_size, int64_t tv_sec, int32_t tv_nsec)
{
    time_t realtime_time;
    struct tm utc_tm;

    if (tv_nsec < 0 || tv_nsec >= (int32_t)NSEC_PER_SEC)
    {
        clock_write_fallback(buf, buf_size, s_iso8601_utc_fallback);
        return -1;
    }

    realtime_time = (time_t)tv_sec;

    if (com_util_gmtime(&utc_tm, &realtime_time) != 0 ||
        clock_format_iso8601_utc_from_tm(buf, buf_size, &utc_tm, tv_nsec) != 0)
    {
        clock_write_fallback(buf, buf_size, s_iso8601_utc_fallback);
        return -1;
    }

    return 0;
}

/* doxygen コメントは、ヘッダーに記載 */
void com_util_get_realtime_deadline_ms(uint64_t timeout_ms, struct timespec *abs_timeout)
{
    int64_t deadline_sec;
    int32_t deadline_nsec;

    com_util_get_realtime(&deadline_sec, &deadline_nsec);

    deadline_sec += (int64_t)(timeout_ms / MSEC_PER_SEC);
    deadline_nsec += (int32_t)((timeout_ms % MSEC_PER_SEC) * NSEC_PER_MSEC);

    if (deadline_nsec >= NSEC_PER_SEC)
    {
        deadline_sec += 1;
        deadline_nsec -= (int32_t)NSEC_PER_SEC;
    }

    clock_fill_timespec(abs_timeout, deadline_sec, deadline_nsec);
}
