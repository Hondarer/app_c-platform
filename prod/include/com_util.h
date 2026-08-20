/**
 *******************************************************************************
 *  @file           com_util.h
 *  @brief          com_util ライブラリの公開 API をまとめて取り込みます。
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
 *  また、以下のヘッダーは特定プラットフォーム専用の API のみを宣言するため、
 *  このアンブレラ ヘッダーには含まれていません。
 *  利用箇所を `#if defined(PLATFORM_*)` で明示する前提のもと、個別にインクルードしてください。
 *  - `<com_util/trace/etw.h>` : Windows 専用 (ETW TraceLogging)。
 *  - `<com_util/trace/eventlog.h>` : Windows 専用 (イベント ログ)。
 *  - `<com_util/trace/syslog.h>` : Linux 専用 (syslog)。
 *  - `<com_util/win32/win32.h>` : Windows 専用 (Win32 API の UTF-8 ラッパー)。
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

/* カテゴリ別サブグループの定義です。各公開ヘッダーは自身のカテゴリへ @ingroup します。 */

/**
 *  @defgroup       COM_UTIL_BASE 基盤 (base)
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @brief          コンパイラ / プラットフォーム判定、DLL エクスポート、結果コードなどの基盤機能です。
 */

/**
 *  @defgroup       COM_UTIL_ARGPARSER 引数解析 (argparser)
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @brief          コマンド ライン引数の登録と解析を行う API です。
 */

/**
 *  @defgroup       COM_UTIL_CLOCK 時刻 (clock)
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @brief          単調時刻・実時刻の取得、標準時刻型 com_util_timespec とその演算 API です。
 */

/**
 *  @defgroup       COM_UTIL_COMPRESS 圧縮 (compress)
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @brief          データの圧縮・伸張 API です。
 */

/**
 *  @defgroup       COM_UTIL_CONSOLE コンソール (console)
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @brief          コンソールの初期化と UTF-8 出力を抽象化する API です。
 */

/**
 *  @defgroup       COM_UTIL_CRT CRT 抽象 (crt)
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @brief          C 標準ライブラリ / POSIX 関数のプラットフォーム差異を抽象化する API です。
 */

/**
 *  @defgroup       COM_UTIL_CRYPTO 暗号 (crypto)
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @brief          AES-256-GCM による暗号化・復号 API です。
 */

/**
 *  @defgroup       COM_UTIL_HASHTABLE ハッシュ テーブル (hashtable)
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @brief          固定長スロットと遅延削除を持つハッシュ テーブル API です。
 */

/**
 *  @defgroup       COM_UTIL_MMAP メモリ マップド ファイル (mmap)
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @brief          メモリ マップド ファイルの割り当てと共有 API です。
 */

/**
 *  @defgroup       COM_UTIL_NET ネットワーク (net)
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @brief          IPv4 ソケットの生成、接続、送受信、アドレス解決 API です。
 */

/**
 *  @defgroup       COM_UTIL_PROMPT プロンプト (prompt)
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @brief          対話プロンプトの入力・表示 API です。
 */

/**
 *  @defgroup       COM_UTIL_REGEX 正規表現 (regex)
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @brief          UTF-8 文字列に対する正規表現の照合 API です。
 */

/**
 *  @defgroup       COM_UTIL_RUNTIME ランタイム (runtime)
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @brief          プロセス、モジュール、シャットダウン、メモリ ロックなどのランタイム支援 API です。
 */

/**
 *  @defgroup       COM_UTIL_SYNC 同期 (sync)
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @brief          スレッド間・プロセス間の同期プリミティブ API です。
 */

/**
 *  @defgroup       COM_UTIL_TRACE トレース (trace)
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @brief          レベル別トレース出力と各バックエンド (ファイル、syslog、ETW、イベント ログ) の API です。
 */

/**
 *  @defgroup       COM_UTIL_WIN32 Win32 UTF-8 ラッパー (win32)
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @brief          Win32 API を UTF-8 文字列で呼び出す U サフィックスのラッパー API です。
 */

/**
 *  @defgroup       COM_UTIL_TEST テスト支援 (test)
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @brief          テストからの利用を想定した補助 API です。
 */

#include <com_util/base/compiler.h>
#include <com_util/base/platform.h>
#include <com_util/base/dll_exports.h>
#include <com_util/base/error.h>
#include <com_util/base/error_message.h>
#include <com_util/base/result.h>
#include <com_util/base/windows_sdk.h>

#include <com_util/argparser/argparser.h>

#include <com_util/clock/clock.h>
#include <com_util/clock/timespec.h>
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
#include <com_util/crt/wchar_conv.h>

#include <com_util/crypto/crypto.h>
#include <com_util/crypto/random.h>

#include <com_util/hashtable/hashtable.h>

#include <com_util/mmap/mmap.h>

#include <com_util/net/byteorder.h>
#include <com_util/net/endpoint.h>
#include <com_util/net/socket.h>

#include <com_util/prompt/pinned_prompt.h>
#include <com_util/prompt/prompt.h>

#include <com_util/regex/regex.h>

#include <com_util/runtime/module.h>
#include <com_util/runtime/memory_lock.h>
#include <com_util/runtime/process.h>
#include <com_util/runtime/shutdown.h>
#include <com_util/runtime/sym_loader.h>

#include <com_util/sync/sync.h>

#include <com_util/trace/trace_file.h>
#include <com_util/trace/tracer.h>

#endif /* COM_UTIL_H */
