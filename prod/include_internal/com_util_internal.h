/**
 *******************************************************************************
 *  @file           com_util_internal.h
 *  @brief          com_util ライブラリの内部傘ヘッダー (内部 API ひとまとめ、実装出力ヘッダーを除く)。
 *  @author         Tetsuo Honda
 *  @date           2026/05/21
 *  @version        1.0.0
 *
 *  com_util ライブラリの内部ヘッダーを 1 つにまとめます。\n
 *  ライブラリ内部実装から `<com_util_internal.h>` 1 行で全内部 API にアクセスできます。
 *
 *  @par            除外ヘッダー
 *  以下のヘッダーはインクルードされた翻訳単位に実装コードを出力するため、
 *  この傘ヘッダーには含まれていません。必要な場合は個別にインクルードしてください。
 *  - `<com_util/crt/crt_internal.h>` :
 *    Windows 限定で `static` 関数 `com_util_utf8_to_wpath()` /
 *    `com_util_wpath_to_utf8()` を定義します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef COM_UTIL_INTERNAL_H
#define COM_UTIL_INTERNAL_H

#include <com_util/console/console_internal.h>

#include <com_util/crt/path_format.h>

#include <com_util/prompt/prompt_edit.h>
#include <com_util/prompt/prompt_internal.h>

#include <com_util/trace/tracer_internal.h>
#include <com_util/trace/backends/etw/etw_internal.h>
#include <com_util/trace/backends/file/trace_file_internal.h>
#include <com_util/trace/backends/syslog/syslog_internal.h>

#endif /* COM_UTIL_INTERNAL_H */
