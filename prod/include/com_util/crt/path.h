/**
 *******************************************************************************
 *  @file           path.h
 *  @brief          CRT 抽象層で使用するパス関連定義。
 *  @author         Tetsuo Honda
 *  @date           2026/04/22
 *
 *  プラットフォームに依存せず使用できるパス関連の定数と API を提供します。
 *
 *  **パスセパレータ方針**\n
 *  本ライブラリはパスセパレータを全プラットフォームで @ref PLATFORM_PATH_SEP (`"/"`) に統一します。
 *
 *  - **入力パス** (呼び出し元が渡す): `"/"` をそのまま渡してください。
 *    Windows の CRT/Win32 API は wchar_t パス内の `'/'` を受け付けるため変換不要です。
 *  - **出力パス** (`path_out` など): Windows API が返す `'\'` は内部で `'/'` に正規化して
 *    呼び出し元に返します。常に `'/'` 区切りのパスを受け取れます。
 *  - **パス構築**: ライブラリ内部でセパレータが必要な場合は @ref PLATFORM_PATH_SEP を使用します。
 *  - **外部由来パス**: 環境変数や設定ファイルから取得したパスは
 *    com_util_normalize_path_sep() で正規化できます。
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

#ifndef COM_UTIL_CRT_PATH_H
#define COM_UTIL_CRT_PATH_H

#include <com_util/base/platform.h>
#include <com_util/com_util_export.h>
#include <stddef.h>

#ifdef DOXYGEN
    /**
     *  @brief          OS 固有のパス最大長。
     *                  Linux では PATH_MAX、Windows では MAX_PATH に対応する値を使用する。
     */
    #define PLATFORM_PATH_MAX 4096
#else /* !DOXYGEN */
    #if defined(PLATFORM_LINUX)
        #include <limits.h>
        #define PLATFORM_PATH_MAX PATH_MAX
    #elif defined(PLATFORM_WINDOWS)
        #include <com_util/base/windows_sdk.h>
        #define PLATFORM_PATH_MAX MAX_PATH
    #endif /* PLATFORM_ */
#endif     /* DOXYGEN */

/**
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @{
 */

/**
 *  @brief          ファイルパス区切り文字列。全プラットフォームで `"/"` に統一します。
 */
#define PLATFORM_PATH_SEP "/"
/**
 *  @brief          ファイルパス区切り文字 (char 型)。全プラットフォームで `'/'` に統一します。
 */
#define PLATFORM_PATH_SEP_CHR '/'

#define COM_UTIL_PATH_CONCAT_COUNT_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, count, \
                                        ...) \
    count
#define COM_UTIL_PATH_CONCAT_COUNT(...) \
    COM_UTIL_PATH_CONCAT_COUNT_IMPL(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

/**
 *  @brief          パス断片を指定順にそのまま連結します。
 *
 *  @param[out]     path_out   連結結果の格納先。NULL を渡してはなりません。
 *  @param[in]      path_size  @p path_out のサイズ (バイト)。0 を渡してはなりません。
 *  @param[out]     errno_out  エラー詳細の格納先。NULL 可。
 *  @param[in]      ...        連結する UTF-8 文字列断片。少なくとも 1 つ必要です。
 *
 *  断片は自動補正せず、そのまま連結されます。\n
 *  パス区切り文字が必要な場合は @ref PLATFORM_PATH_SEP を明示的に指定してください。
 */
#define com_util_path_concat(path_out, path_size, errno_out, ...) \
    com_util_path_concat_n((path_out), (path_size), (errno_out), COM_UTIL_PATH_CONCAT_COUNT(__VA_ARGS__), __VA_ARGS__)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          パス文字列内の '\\' を '/' に正規化します (インプレース)。
     *  @param[in,out]  path  正規化対象のパス文字列 (UTF-8)。NULL を渡してはなりません。
     *  @return         path を返します (連鎖呼び出し用)。
     *
     *  環境変数や設定ファイルから読み取ったパスに Windows スタイルの '\\' が含まれる場合に
     *  使用します。\n
     *  本ライブラリが出力するパス (@p path_out 等) はすでに @ref PLATFORM_PATH_SEP (`"/"`) に
     *  正規化済みのため、本関数を呼び出す必要はありません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一 @p path を複数スレッドから同時に書き換えないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT char *COM_UTIL_API com_util_normalize_path_sep(char *path);

    /**
     *  @brief          パスを絶対化し、区切り文字を '/' に正規化して返します。
     *  @param[out]     path_out    絶対化済みパス (UTF-8) の格納先。NULL を渡してはなりません。
     *  @param[in]      path_size   @p path_out のサイズ (バイト)。0 を渡してはなりません。
     *  @param[out]     errno_out   エラー詳細の格納先。NULL 可。成功時は変更しません。
     *  @param[in]      path        入力パス (UTF-8)。NULL および空文字は渡してはなりません。
     *  @return         成功時は 0、失敗時は -1 を返します。
     *
     *  相対パスはカレントディレクトリ基準で絶対化します。\n
     *  Linux では realpath() による symlink 解決を可能な範囲で試み、失敗した場合は
     *  '.' / '..' を解消した絶対パス文字列を返します。\n
     *  Windows では GetFullPathNameW() により絶対化し、返却値は常に
     *  @ref PLATFORM_PATH_SEP (`"/"`) 区切りへ正規化されます。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。相対パスを渡した場合、他スレッドがカレント ディレクトリを変更すると解決結果が不定になります。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_path_get_full(char *path_out, size_t path_size, int *errno_out,
                                                            const char *path);

    /**
     *  @brief          2 つのパスが同じ実体を指すか比較します。
     *  @param[in]      lhs        比較対象の 1 つ目のパス (UTF-8)。NULL および空文字は渡してはなりません。
     *  @param[in]      rhs        比較対象の 2 つ目のパス (UTF-8)。NULL および空文字は渡してはなりません。
     *  @param[out]     errno_out  エラー詳細の格納先。NULL 可。成功時は変更しません。
     *  @return         一致時は 1、不一致時は 0、失敗時は -1 を返します。
     *
     *  内部でそれぞれのパスに対して com_util_path_get_full() を呼び、絶対化と
     *  区切り文字正規化を行ったうえで比較します。\n
     *  Windows ではファイルシステムの慣習に合わせて大小文字を区別せず比較します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。相対パスを渡した場合、他スレッドがカレント ディレクトリを変更すると比較結果が不定になります。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_paths_equal(const char *lhs, const char *rhs, int *errno_out);

    /**
     *  @brief          プラットフォームの一時ディレクトリのパスを取得します。
     *  @param[out]     path_out    一時ディレクトリの絶対パス (UTF-8) の格納先。
     *                              末尾パス区切り文字 (@ref PLATFORM_PATH_SEP_CHR) は付きません。
     *                              NULL を渡してはなりません。
     *  @param[in]      path_size   @p path_out のサイズ (バイト)。0 を渡してはなりません。
     *  @param[out]     errno_out   エラー詳細の格納先。NULL 可。成功時は変更しません。
     *  @return         成功時は 0、失敗時は -1 を返します。
     *
     *  Linux 環境では環境変数 @c TMPDIR を参照し、未設定または空の場合は @c "/tmp" を使用します。\n
     *  Windows 環境では @c GetTempPathW() で取得したパスを UTF-8 に変換して使用します。\n
     *  出力パスは常に @ref PLATFORM_PATH_SEP (`"/"`) 区切りで正規化されており、末尾の区切り文字は含まれません。\n
     *  ファイルパスを構築する際は @ref PLATFORM_PATH_SEP を挟んでください。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_get_temp_dir(char *path_out, size_t path_size, int *errno_out);

    /**
     *  @brief          パス断片を指定順にそのまま連結します。
     *  @param[out]     path_out    連結結果の格納先。NULL を渡してはなりません。
     *  @param[in]      path_size   @p path_out のサイズ (バイト)。0 を渡してはなりません。
     *  @param[out]     errno_out   エラー詳細の格納先。NULL 可。
     *  @param[in]      part_count  連結する断片数。1 以上を渡してください。
     *  @param[in]      ...         連結する UTF-8 文字列断片。
     *  @return         成功時は 0、失敗時は -1 を返します。
     *
     *  断片は自動補正せず、そのまま連結されます。\n
     *  いずれかの断片が NULL、または @p part_count が 0 の場合は EINVAL を返します。\n
     *  結果が @p path_out に収まらない場合は ENAMETOOLONG を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_path_concat_n(char *path_out, size_t path_size, int *errno_out,
                                                            size_t part_count, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_CRT_PATH_H */
