/**
 *******************************************************************************
 *  @file           sys/stat.h
 *  @brief          stat 系の CRT 関数を抽象化する API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/04/22
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_CRT_SYS_STAT_H
#define COM_UTIL_CRT_SYS_STAT_H

#include <stdarg.h>
#include <sys/stat.h>
#include <com_util/base/compiler.h>
#include <com_util/base/error.h>
#include <com_util/base/platform.h>
#include <com_util/base/result.h>
#include <com_util/com_util_export.h>

/**
 *  @ingroup        COM_UTIL_CRT
 *  @{
 */

#ifndef COM_UTIL_FILE_STAT_T_DEFINED
    #define COM_UTIL_FILE_STAT_T_DEFINED
    /* OS / SDK が定義する stat 型の alias であるため、_t サフィックスを残す。
     * see: コーディング規範「予約識別子の回避」の例外 */
    #ifdef DOXYGEN
/**
 *  @brief  プラットフォーム固有のファイル情報構造体
 *          (Linux: `struct stat`、Windows: `struct _stat64`)。
 */
typedef struct stat com_util_file_stat_t;
    #elif defined(PLATFORM_LINUX)
typedef struct stat com_util_file_stat_t;
    #elif defined(PLATFORM_WINDOWS)
typedef struct _stat64 com_util_file_stat_t;
    #endif /* PLATFORM_ */
#endif     /* COM_UTIL_FILE_STAT_T_DEFINED */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          UTF-8 パスのファイル情報を取得します (`stat` / `_wstat64` ラッパー)。
     *  @param[out]     buf   ファイル情報の格納先。NULL を渡してはなりません。
     *  @param[out]     detail_out  エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *                  NULL 以外を指定した場合、成功時は空の値を格納します。
     *  @param[in]      path  対象ファイルのパス (UTF-8)。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  Linux では `stat`、Windows では `_wstat64` を使用します。\n
     *  Windows の `_wstat64` は時刻欄を現地時刻経由で `time_t` へ変換するため、
     *  タイムゾーン設定によっては Unix epoch UTC と秒部がずれます。
     *  本関数は `GetFileAttributesExW` が返す UTC の `FILETIME` から秒部を取り直し、
     *  `st_atime` / `st_mtime` / `st_ctime` を Linux の `stat` と同じ Unix epoch UTC に揃えます。\n
     *  `GetFileAttributesExW` に失敗した場合は `_wstat64` の時刻欄をそのまま返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_stat(com_util_file_stat_t *buf, com_util_error *detail_out,
                                                   const char *path);

    /**
     *  @brief          UTF-8 パスのディレクトリを作成します (`mkdir` / `_wmkdir` ラッパー)。
     *  @param[in]      path  作成するディレクトリのパス (UTF-8)。NULL を渡してはなりません。
     *  @param[out]     detail_out  エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *                  NULL 以外を指定した場合、成功時は空の値を格納します。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_mkdir(const char *path, com_util_error *detail_out);

    /**
     *  @brief          UTF-8 パスのディレクトリを、欠けている中間ディレクトリも
     *                  含めて再帰的に作成します (`mkdir -p` 相当)。
     *  @param[in]      path  作成するディレクトリのパス (UTF-8)。NULL を渡してはなりません。
     *  @param[out]     detail_out  エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *                  NULL 以外を指定した場合、成功時は空の値を格納します。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  すでに存在するディレクトリは成功として扱います (べき等)。\n
     *  中間ディレクトリが欠けている場合はすべて生成します。\n
     *  他プロセスによる競合生成は成功として扱います。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_makedirs(const char *path, com_util_error *detail_out);

    /**
     *  @brief          UTF-8 パスの空のディレクトリを削除します (`rmdir` / `_wrmdir` ラッパー)。
     *  @param[in]      path  削除するディレクトリのパス (UTF-8)。NULL を渡してはなりません。
     *  @param[out]     detail_out  エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *                  NULL 以外を指定した場合、成功時は空の値を格納します。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  ディレクトリが空でない場合、存在しない場合はいずれも失敗します。\n
     *  com_util_makedirs() のように中間ディレクトリを再帰的に削除することはありません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_rmdir(const char *path, com_util_error *detail_out);

    /**
     *  @brief          書式指定パスのファイル情報を取得します。
     *  @param[out]     buf     ファイル情報の格納先。NULL を渡してはなりません。
     *  @param[out]     detail_out  エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *                  NULL 以外を指定した場合、成功時は空の値を格納します。
     *  @param[in]      format  パスを構築する printf 形式の書式文字列。
     *  @param[in]      ...     書式引数。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_stat_fmt(com_util_file_stat_t *buf, com_util_error *detail_out,
                                                       const char *format, ...)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 3, 4)))
#endif /* COMPILER_GCC */
        ;

    /**
     *  @brief          書式指定パスのファイル情報を取得します (`com_util_stat_fmt` の `va_list` 版)。
     *  @param[out]     buf     ファイル情報の格納先。NULL を渡してはなりません。
     *  @param[out]     detail_out  エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *                  NULL 以外を指定した場合、成功時は空の値を格納します。
     *  @param[in]      format  パスを構築する printf 形式の書式文字列。
     *  @param[in]      args    書式引数リスト。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_vstat_fmt(com_util_file_stat_t *buf, com_util_error *detail_out,
                                                        const char *format, va_list args)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 3, 0)))
#endif /* COMPILER_GCC */
        ;

    /**
     *  @brief          書式指定パスのディレクトリを作成します。
     *  @param[out]     detail_out  エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *                  NULL 以外を指定した場合、成功時は空の値を格納します。
     *  @param[in]      format  パスを構築する printf 形式の書式文字列。
     *  @param[in]      ...     書式引数。
     *  @return         @ref COM_UTIL_OK または @ref COM_UTIL_ERR_UNKNOWN を返します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_mkdir_fmt(com_util_error *detail_out, const char *format, ...)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 2, 3)))
#endif /* COMPILER_GCC */
        ;

    /**
     *  @brief          書式指定パスのディレクトリを作成します (`com_util_mkdir_fmt` の `va_list` 版)。
     *  @param[out]     detail_out  エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *                  NULL 以外を指定した場合、成功時は空の値を格納します。
     *  @param[in]      format  パスを構築する printf 形式の書式文字列。
     *  @param[in]      args    書式引数リスト。
     *  @return         @ref COM_UTIL_OK または @ref COM_UTIL_ERR_UNKNOWN を返します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_vmkdir_fmt(com_util_error *detail_out, const char *format, va_list args)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 2, 0)))
#endif /* COMPILER_GCC */
        ;

    /**
     *  @brief          ファイル情報が通常ファイルを表すかを返します。
     *  @param[in]      file_stat  判定するファイル情報。NULL 可。
     *  @return         通常ファイルの場合は 1、それ以外は 0 を返します。
     *
     *  `com_util_stat()` で取得したファイル情報の種別を判定します。\n
     *  ディレクトリ、デバイス ファイル、その他の特殊ファイルはいずれも 0 になります。\n
     *  引数の値だけを参照し、ファイル システムへはアクセスしません。
     *  そのため本関数は共通結果コードの適用対象外であり、判定結果をそのまま返します。
     *
     *  Linux では `S_ISREG`、Windows では `_S_IFMT` と `_S_IFREG` の比較で判定します。\n
     *  `com_util_file_stat_t` は OS が定義する構造体の別名であるため、
     *  種別の判定に必要なマクロの差異を本関数が吸収します。
     *
     *  @attention      @p file_stat が NULL の場合は 0 を返します。
     *                  引数の妥当性を確認する用途には使用できません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  引数の値だけを参照し、共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_file_stat_is_regular(const com_util_file_stat_t *file_stat);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_CRT_SYS_STAT_H */
