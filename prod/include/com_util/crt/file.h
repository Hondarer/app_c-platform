/**
 *******************************************************************************
 *  @file           file.h
 *  @brief          低レベル ファイル I/O 抽象 API。
 *  @author         Tetsuo Honda
 *  @date           2026/04/24
 *
 *  Windows の HANDLE ベース I/O と Linux の fd ベース I/O を共通化し、
 *  UTF-8 パスを受け取る低レベル書き込み用 API を提供します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_CRT_FILE_H
#define COM_UTIL_CRT_FILE_H

#include <stddef.h>
#include <stdint.h>
#include <com_util/base/platform.h>
#include <com_util/com_util_export.h>

#if defined(PLATFORM_WINDOWS)
    #include <com_util/base/windows_sdk.h>
#endif /* PLATFORM_WINDOWS */

/**
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @{
 */

#define COM_UTIL_FILE_OPEN_CREATE        (1 << 0) /**< ファイルが存在しない場合に新規作成する。 */
#define COM_UTIL_FILE_OPEN_TRUNCATE      (1 << 1) /**< 既存ファイルを開く際に内容を切り詰める。 */
#define COM_UTIL_FILE_OPEN_APPEND        (1 << 2) /**< 書き込みをファイル末尾に追記する。 */
#define COM_UTIL_FILE_OPEN_WRITE_THROUGH (1 << 3) /**< 書き込みをバッファリングせずディスクに直接書き出す。 */
#define COM_UTIL_FILE_OPEN_SHARE_READ    (1 << 4) /**< 他プロセスからの読み取り共有を許可する。 */
#define COM_UTIL_FILE_OPEN_SHARE_DELETE  (1 << 5) /**< 他プロセスからの削除を許可する。 */

/**
 *  @brief  ファイル ハンドルの抽象化構造体 (Linux の fd、Windows の HANDLE を保持)。
 */
typedef struct com_util_file
{
#if defined(PLATFORM_LINUX)
    int handle;
#elif defined(PLATFORM_WINDOWS)
    HANDLE handle;
#endif /* PLATFORM_ */
} com_util_file;

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          `com_util_file` 構造体を無効状態に初期化します。
     *  @param[out]     file  初期化対象の構造体。NULL を渡してはなりません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一 @p file を複数スレッドから同時に書き換えないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_file_init(com_util_file *file);

    /**
     *  @brief          UTF-8 パスでファイルを開きます。
     *  @param[in,out]  file   オープン結果の格納先。NULL を渡してはなりません。
     *                         すでにオープン済みの場合は先にクローズしてから開き直します。
     *  @param[in]      path   開くファイルのパス (UTF-8)。NULL を渡してはなりません。
     *  @param[in]      flags  オープン フラグ (@ref COM_UTIL_FILE_OPEN_CREATE 等の OR 結合)。
     *                         負値を渡した場合は -1 を返します。
     *  @return         成功時は 0、失敗時は -1 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_file_open(com_util_file *file, const char *path, int flags);

    /**
     *  @brief          ファイルにバイト列を書き込みます。
     *  @param[in]      file  書き込み対象のファイル。NULL を渡してはなりません。
     *  @param[in]      buf   書き込むデータ。NULL を渡してはなりません。
     *  @param[in]      len   書き込むバイト数。
     *  @return         成功時は 0、失敗時は -1 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一 @p file への並行書き込みは呼び出し側で同期してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_file_write(com_util_file *file, const void *buf, size_t len);

    /**
     *  @brief          ファイル サイズを取得します。
     *  @param[in]      file      対象のファイル。NULL を渡してはなりません。
     *  @param[out]     size_out  サイズ (バイト) の格納先。NULL を渡してはなりません。
     *  @return         成功時は 0、失敗時は -1 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_file_get_size(const com_util_file *file, size_t *size_out);

    /**
     *  @brief          ファイルを閉じます。
     *                  `com_util_file_init()` で初期化済みの無効ハンドルには何もしません。
     *  @param[in]      file  閉じるファイル。NULL を渡してはなりません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一 @p file を複数スレッドから同時に操作しないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_file_close(com_util_file *file);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_CRT_FILE_H */
