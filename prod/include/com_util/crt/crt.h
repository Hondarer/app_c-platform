/**
 *******************************************************************************
 *  @file           crt.h
 *  @brief          CRT 抽象 API の集約ヘッダーです。
 *  @author         Tetsuo Honda
 *  @date           2026/04/24
 *
 *  `com_util/crt` 配下の公開ヘッダーをまとめて取り込む標準入口です。\n
 *  CRT 抽象 API を広く利用する場合は、本ヘッダーの利用を推奨します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_CRT_CRT_H
#define COM_UTIL_CRT_CRT_H

/**
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @{
 */

#include <com_util/crt/path.h>
#include <com_util/crt/fcntl.h>
#include <com_util/crt/file.h>
#include <com_util/crt/stdio.h>
#include <com_util/crt/stdlib.h>
#include <com_util/crt/string.h>
#include <com_util/crt/time.h>
#include <com_util/crt/unistd.h>
#include <com_util/crt/sys/stat.h>

/** @} */

#endif /* COM_UTIL_CRT_CRT_H */
