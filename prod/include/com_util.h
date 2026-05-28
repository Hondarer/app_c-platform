/**
 *******************************************************************************
 *  @file           com_util.h
 *  @brief          com_util ライブラリの公開アンブレラ ヘッダー。
 *  @author         Tetsuo Honda
 *  @date           2026/05/21
 *  @version        1.0.0
 *
 *  com_util ライブラリの公開ヘッダーを 1 つにまとめたヘッダーです。\n
 *  利用者は `#include <com_util.h>` で本ライブラリの全公開 API にアクセスできます。
 *
 *  アンブレラ ヘッダーは利便性と引き換えにコンパイル時間がかかります。\n
 *  個別ヘッダーを利用するか、アンブレラ ヘッダーを利用するかは利用者にて選択してください。
 *
 *  @par            除外ヘッダー
 *  以下のヘッダーはインクルードされた翻訳単位に実装コードを出力するため、
 *  このアンブレラ ヘッダーには含まれていません。必要な場合は個別にインクルードしてください。
 *  - `<com_util/base/shared_lib_lifecycle.h>` :
 *    `__attribute__((constructor))` / `DllMain` を定義し、
 *    利用側に `onLoad()` / `onUnload()` の実装を要求します。
 *  - `<com_util/test/syslog_test.h>` :
 *    Linux 限定で `static` 関数 `syslog_test_fd_write__()` を定義します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_H
#define COM_UTIL_H

/**
 *  @defgroup       COM_UTIL_PUBLIC_API 公開 API (com_util)
 *  @brief          com_util ライブラリの公開 API です。
 */

#include <com_util/base/compiler.h>
#include <com_util/base/platform.h>
#include <com_util/base/dll_exports.h>
#include <com_util/base/windows_sdk.h>

#include <com_util/clock/clock.h>
#include <com_util/compress/compress.h>
#include <com_util/console/console.h>

#include <com_util/crt/crt.h>
#include <com_util/crt/fcntl.h>
#include <com_util/crt/file.h>
#include <com_util/crt/path.h>
#include <com_util/crt/stdio.h>
#include <com_util/crt/stdlib.h>
#include <com_util/crt/string.h>
#include <com_util/crt/sys/stat.h>
#include <com_util/crt/time.h>
#include <com_util/crt/unistd.h>

#include <com_util/crypto/crypto.h>

#include <com_util/prompt/pinned_prompt.h>
#include <com_util/prompt/prompt.h>

#include <com_util/runtime/module.h>
#include <com_util/runtime/shutdown.h>
#include <com_util/runtime/sym_loader.h>

#include <com_util/sync/sync.h>

#include <com_util/trace/etw.h>
#include <com_util/trace/syslog.h>
#include <com_util/trace/trace_file.h>
#include <com_util/trace/tracer.h>

#endif /* COM_UTIL_H */
