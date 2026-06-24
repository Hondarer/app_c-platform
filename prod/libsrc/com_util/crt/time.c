/**
 *******************************************************************************
 *  @file           time.c
 *  @brief          time 系の CRT 関数を抽象化する API を実装します。
 *
 *  UTC 時刻とローカル時刻への変換をスレッド セーフな OS API へ委譲します。
 *
 *******************************************************************************
 */

#include <com_util/crt/time.h>

#include <string.h>

#include <com_util/base/platform.h>

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_gmtime(struct tm *utc_tm, const time_t *timep)
{
    if (utc_tm == NULL || timep == NULL)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    if (gmtime_r(timep, utc_tm) == NULL)
    {
        memset(utc_tm, 0, sizeof(*utc_tm));
        return -1;
    }
    return 0;
#elif defined(PLATFORM_WINDOWS)
    if (gmtime_s(utc_tm, timep) != 0)
    {
        memset(utc_tm, 0, sizeof(*utc_tm));
        return -1;
    }
    return 0;
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_localtime(struct tm *local_tm, const time_t *timep)
{
    if (local_tm == NULL || timep == NULL)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    if (localtime_r(timep, local_tm) == NULL)
    {
        memset(local_tm, 0, sizeof(*local_tm));
        return -1;
    }
    return 0;
#elif defined(PLATFORM_WINDOWS)
    if (localtime_s(local_tm, timep) != 0)
    {
        memset(local_tm, 0, sizeof(*local_tm));
        return -1;
    }
    return 0;
#endif /* PLATFORM_ */
}
