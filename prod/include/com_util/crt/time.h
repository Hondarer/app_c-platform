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

    COM_UTIL_EXPORT int COM_UTIL_API com_util_gmtime(struct tm *utc_tm,
                                                      const time_t *timep);
    COM_UTIL_EXPORT int COM_UTIL_API com_util_localtime(struct tm *local_tm,
                                                         const time_t *timep);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COM_UTIL_CRT_TIME_H */
