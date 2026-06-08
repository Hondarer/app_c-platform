/**
 *******************************************************************************
 *  @file           com_util_internal.h
 *  @brief          com_util ライブラリの公開 + 内部アンブレラ ヘッダー。
 *  @author         Tetsuo Honda
 *  @date           2026/05/21
 *  @version        1.0.0
 *
 *  com_util ライブラリの内部ヘッダーを 1 つにまとめたヘッダーです。\n
 *  利用者は `#include <com_util_internal.h>` で本ライブラリの全公開 + 全内部 API にアクセスできます。
 *
 *  アンブレラ ヘッダーは利便性と引き換えにコンパイル時間がかかります。\n
 *  個別ヘッダーを利用するか、アンブレラ ヘッダーを利用するかは利用者にて選択してください。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_INTERNAL_H
#define COM_UTIL_INTERNAL_H

#include <com_util.h> /* 内部 API で公開定数、公開型、公開関数などに依存している可能性を考慮 */

#include <com_util/console/console_internal.h>

#include <com_util/crt/path_format.h>

#include <com_util/prompt/prompt_edit.h>
#include <com_util/prompt/prompt_internal.h>

#include <com_util/trace/tracer_internal.h>
#include <com_util/trace/backends/etw/etw_internal.h>
#include <com_util/trace/backends/file/trace_file_internal.h>
#include <com_util/trace/backends/syslog/syslog_internal.h>

#endif /* COM_UTIL_INTERNAL_H */
