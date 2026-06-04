/**
 *******************************************************************************
 *  @file           stdio.h
 *  @brief          stdio 系 CRT 抽象 API。
 *  @author         Tetsuo Honda
 *  @date           2026/04/22
 *
 *  C 標準ファイル I/O 関数をプラットフォーム差異なしで使用できるラッパーを提供します。\n
 *  ファイルパスを受け取る関数は UTF-8 文字列として扱い、Windows では内部で
 *  Unicode (_W 系関数) に変換します。\n
 *  出力パス (@p path_out 等) はプラットフォームによらず @ref PLATFORM_PATH_SEP (`"/"`) に
 *  統一されます。パスセパレータの詳細な方針は @ref path.h を参照してください。
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

#ifndef COM_UTIL_CRT_STDIO_H
#define COM_UTIL_CRT_STDIO_H

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <com_util/base/compiler.h>
#include <com_util/com_util_export.h>

/**
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          UTF-8 パスでファイルを開きます (`fopen` ラッパー)。
     *  @param[in]      path       開くファイルのパス (UTF-8)。NULL を渡してはなりません。
     *  @param[in]      modes      fopen 互換のモード文字列 ("r"、"w"、"rb" など)。NULL を渡してはなりません。
     *  @param[out]     errno_out  エラー詳細の格納先。NULL 可。失敗時に errno を格納します。
     *  @return         成功時は FILE*、失敗時は NULL を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT FILE *COM_UTIL_API com_util_fopen(const char *path, const char *modes, int *errno_out);

    /**
     *  @brief          UTF-8 パスのファイルを削除します (`remove` / `_wremove` ラッパー)。
     *  @param[in]      path  削除するファイルのパス (UTF-8)。NULL を渡してはなりません。
     *  @return         成功時は 0、失敗時は -1 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_remove(const char *path);

    /**
     *  @brief          UTF-8 パスのファイルを改名します (`rename` / `_wrename` ラッパー)。
     *  @param[in]      oldpath  変更前のパス (UTF-8)。NULL を渡してはなりません。
     *  @param[in]      newpath  変更後のパス (UTF-8)。NULL を渡してはなりません。
     *  @return         成功時は 0、失敗時は -1 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_rename(const char *oldpath, const char *newpath);

    /**
     *  @brief          ストリームを閉じます (`fclose` ラッパー)。
     *  @param[in]      stream  閉じるストリーム。NULL を渡してはなりません。
     *  @return         成功時は 0、失敗時は EOF を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一 @p stream を複数スレッドから同時に操作しないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_fclose(FILE *stream);

    /**
     *  @brief          ストリームからデータを読み取ります (`fread` ラッパー)。
     *  @param[out]     ptr     読み取ったデータの格納先。NULL を渡してはなりません。
     *  @param[in]      size    各要素のサイズ (バイト)。
     *  @param[in]      count   読み取る要素数。
     *  @param[in]      stream  読み取り元のストリーム。NULL を渡してはなりません。
     *  @return         読み取った要素数を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一 @p stream を複数スレッドで共有する場合の整合性は CRT および呼び出し側の同期に依存します。
     */
    COM_UTIL_EXPORT size_t COM_UTIL_API com_util_fread(void *ptr, size_t size, size_t count, FILE *stream);

    /**
     *  @brief          ストリームへデータを書き込みます (`fwrite` ラッパー)。
     *  @param[in]      ptr     書き込むデータ。NULL を渡してはなりません。
     *  @param[in]      size    各要素のサイズ (バイト)。
     *  @param[in]      count   書き込む要素数。
     *  @param[in]      stream  書き込み先のストリーム。NULL を渡してはなりません。
     *  @return         書き込んだ要素数を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一 @p stream を複数スレッドで共有する場合の整合性は CRT および呼び出し側の同期に依存します。
     */
    COM_UTIL_EXPORT size_t COM_UTIL_API com_util_fwrite(const void *ptr, size_t size, size_t count, FILE *stream);

    /**
     *  @brief          ストリームから 1 行読み取ります (`fgets` ラッパー)。
     *  @param[out]     buf     読み取ったデータの格納先。NULL を渡してはなりません。
     *  @param[in]      size    @p buf のサイズ (バイト)。
     *  @param[in]      stream  読み取り元のストリーム。NULL を渡してはなりません。
     *  @return         成功時は @p buf、EOF またはエラー時は NULL を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一 @p stream を複数スレッドで共有する場合の整合性は CRT および呼び出し側の同期に依存します。
     */
    COM_UTIL_EXPORT char *COM_UTIL_API com_util_fgets(char *buf, int size, FILE *stream);

    /**
     *  @brief          ストリームへ文字列を書き込みます (`fputs` ラッパー)。
     *  @param[in]      str     書き込む文字列。NULL を渡してはなりません。
     *  @param[in]      stream  書き込み先のストリーム。NULL を渡してはなりません。
     *  @return         成功時は非負値、失敗時は EOF を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一 @p stream を複数スレッドで共有する場合の整合性は CRT および呼び出し側の同期に依存します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_fputs(const char *str, FILE *stream);

    /**
     *  @brief          ストリームへ書式化出力します (`fprintf` ラッパー)。
     *  @param[in]      stream  出力先のストリーム。NULL を渡してはなりません。
     *  @param[in]      format  printf 形式の書式文字列。NULL を渡してはなりません。
     *  @param[in]      ...     書式引数。
     *  @return         書き込んだ文字数を返します。失敗時は負値を返します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_fprintf(FILE *stream, const char *format, ...)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 2, 3)))
#endif /* COMPILER_GCC */
        ;

    /**
     *  @brief          ストリームへ書式化出力します (`com_util_fprintf` の `va_list` 版)。
     *  @param[in]      stream  出力先のストリーム。NULL を渡してはなりません。
     *  @param[in]      format  printf 形式の書式文字列。NULL を渡してはなりません。
     *  @param[in]      args    書式引数リスト。
     *  @return         書き込んだ文字数を返します。失敗時は負値を返します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_vfprintf(FILE *stream, const char *format, va_list args)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 2, 0)))
#endif /* COMPILER_GCC */
        ;

    /**
     *  @brief          ストリームのバッファをフラッシュします (`fflush` ラッパー)。
     *  @param[in]      stream  フラッシュするストリーム。NULL を渡してはなりません。
     *  @return         成功時は 0、失敗時は EOF を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一 @p stream を複数スレッドで共有する場合の整合性は CRT および呼び出し側の同期に依存します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_fflush(FILE *stream);

    /**
     *  @brief          ストリームの EOF フラグを確認します (`feof` ラッパー)。
     *  @param[in]      stream  確認するストリーム。NULL を渡してはなりません。
     *  @return         EOF フラグが立っている場合は非 0、それ以外は 0 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一 @p stream を複数スレッドで共有する場合の整合性は CRT および呼び出し側の同期に依存します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_feof(FILE *stream);

    /**
     *  @brief          ストリームのエラーフラグを確認します (`ferror` ラッパー)。
     *  @param[in]      stream  確認するストリーム。NULL を渡してはなりません。
     *  @return         エラーフラグが立っている場合は非 0、それ以外は 0 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一 @p stream を複数スレッドで共有する場合の整合性は CRT および呼び出し側の同期に依存します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_ferror(FILE *stream);

    /**
     *  @brief          ストリームの EOF・エラーフラグをクリアします (`clearerr` ラッパー)。
     *  @param[in]      stream  対象のストリーム。NULL を渡してはなりません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一 @p stream を複数スレッドで共有する場合の整合性は CRT および呼び出し側の同期に依存します。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_clearerr(FILE *stream);

    /**
     *  @brief          ストリーム位置を先頭に戻します (`rewind` ラッパー)。
     *  @param[in]      stream  対象のストリーム。NULL を渡してはなりません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一 @p stream を複数スレッドで共有する場合の整合性は CRT および呼び出し側の同期に依存します。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_rewind(FILE *stream);

    /**
     *  @brief          ストリーム位置を移動します (64bit 対応 `fseek` ラッパー)。
     *  @param[in]      stream  対象のストリーム。NULL を渡してはなりません。
     *  @param[in]      offset  移動量 (バイト)。
     *  @param[in]      whence  基点 (SEEK_SET、SEEK_CUR、SEEK_END)。
     *  @return         成功時は 0、失敗時は -1 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一 @p stream を複数スレッドで共有する場合の整合性は CRT および呼び出し側の同期に依存します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_fseek(FILE *stream, int64_t offset, int whence);

    /**
     *  @brief          ストリームの現在位置を取得します (64bit 対応 `ftell` ラッパー)。
     *  @param[in]      stream  対象のストリーム。NULL を渡してはなりません。
     *  @return         成功時は現在位置 (バイト)、失敗時は -1 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。同一 @p stream を複数スレッドで共有する場合の整合性は CRT および呼び出し側の同期に依存します。
     */
    COM_UTIL_EXPORT int64_t COM_UTIL_API com_util_ftell(FILE *stream);

    /**
     *  @brief          書式指定パスでファイルを開きます。
     *  @param[in]      modes      fopen 互換のモード文字列。NULL を渡してはなりません。
     *  @param[out]     errno_out  エラー詳細の格納先。NULL 可。失敗時に errno を格納します。
     *  @param[in]      format     パスを構築する printf 形式の書式文字列。
     *  @param[in]      ...        書式引数。
     *  @return         成功時は FILE*、失敗時は NULL を返します。
     */
    COM_UTIL_EXPORT FILE *COM_UTIL_API com_util_fopen_fmt(const char *modes, int *errno_out, const char *format, ...)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 3, 4)))
#endif /* COMPILER_GCC */
        ;

    /**
     *  @brief          書式指定パスでファイルを開きます (`com_util_fopen_fmt` の `va_list` 版)。
     *  @param[in]      modes      fopen 互換のモード文字列。NULL を渡してはなりません。
     *  @param[out]     errno_out  エラー詳細の格納先。NULL 可。失敗時に errno を格納します。
     *  @param[in]      format     パスを構築する printf 形式の書式文字列。
     *  @param[in]      args       書式引数リスト。
     *  @return         成功時は FILE*、失敗時は NULL を返します。
     */
    COM_UTIL_EXPORT FILE *COM_UTIL_API com_util_vfopen_fmt(const char *modes, int *errno_out, const char *format,
                                                           va_list args)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 3, 0)))
#endif /* COMPILER_GCC */
        ;

    /**
     *  @brief          書式指定パスのファイルを削除します。
     *  @param[in]      format  パスを構築する printf 形式の書式文字列。
     *  @param[in]      ...     書式引数。
     *  @return         成功時は 0、失敗時は -1 を返します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_remove_fmt(const char *format, ...)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 1, 2)))
#endif /* COMPILER_GCC */
        ;

    /**
     *  @brief          書式指定パスのファイルを削除します (`com_util_remove_fmt` の `va_list` 版)。
     *  @param[in]      format  パスを構築する printf 形式の書式文字列。
     *  @param[in]      args    書式引数リスト。
     *  @return         成功時は 0、失敗時は -1 を返します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_vremove_fmt(const char *format, va_list args)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 1, 0)))
#endif /* COMPILER_GCC */
        ;

    /**
     *  @brief          一意な一時ファイルを atomic に作成し、指定されたモードで開きます。
     *  @param[in]      prefix       ファイル名先頭につける識別子 (UTF-8)。NULL 可。
     *                               4 文字以上を渡した場合は先頭 3 文字を採用します。
     *  @param[in]      modes        fopen 互換のモード文字列 ("wb", "w", "w+b" など)。
     *                               NULL を渡した場合は NULL を返し、@p errno_out に EINVAL を格納します。
     *                               一時ファイルは常に新規作成のため "r"/"rb" は意味を持ちませんが、
     *                               API 層での制限は課しません。
     *  @param[out]     path_out     生成された一時ファイル絶対パス (UTF-8) の格納先。
     *  @param[in]      path_size    @p path_out のサイズ (バイト)。PLATFORM_PATH_MAX 以上を推奨します。
     *  @param[out]     errno_out    エラー詳細の格納先。NULL 可。
     *  @return         成功時はオープンされた FILE*、失敗時は NULL を返します。
     *
     *  Linux 環境では TMPDIR (未設定なら "/tmp") に "<prefix>XXXXXX" のテンプレートで
     *  mkostemp() によりファイルを atomic に作成し、その fd を fdopen(@p modes) で FILE* に
     *  変換します。\n
     *  @p modes に "w"/"w+" を指定しても fdopen() の仕様上ファイルの切り詰めは発生しません。
     *  mkostemp() が新規作成したファイルは常に空のため、実用上の影響はありません。\n
     *  Windows 環境では GetTempPathW + GetTempFileNameW でユニーク名を生成し、
     *  _wfopen_s() で指定モードにて開きます。@p path_out は wchar→UTF-8 変換した結果が
     *  格納されます。\n
     *  呼び出し元は不要になったら com_util_fclose() でクローズし、
     *  必要なら com_util_remove() でファイルを削除する責任があります。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。呼び出しごとに独立した一時ファイルを生成します。
     */
    COM_UTIL_EXPORT FILE *COM_UTIL_API com_util_fopen_temp(const char *prefix, const char *modes, char *path_out,
                                                           size_t path_size, int *errno_out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_CRT_STDIO_H */
