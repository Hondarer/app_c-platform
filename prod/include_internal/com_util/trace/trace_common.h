/**
 *******************************************************************************
 *  @file           trace_common.h
 *  @brief          トレース機能の共通ヘルパー (内部共有) を宣言します。
 *
 *  tracer 本体と各バックエンド (file/syslog など) で共通の、
 *  タイムスタンプ解決とトレース レベル表現の変換を提供します。
 *  実装は trace_common.c の 1 箇所に集約します。
 *******************************************************************************
 */
#ifndef COM_UTIL_TRACE_COMMON_H
#define COM_UTIL_TRACE_COMMON_H

#include <stddef.h>

#include <com_util/clock/clock.h>
#include <com_util/trace/tracer.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     *  @brief          タイムスタンプが有効範囲か判定します。
     *  @param[in]      timestamp  判定対象のタイムスタンプ。NULL 可。
     *  @return         有効な場合 1、NULL または範囲外の場合 0。
     */
    int trace_timestamp_is_valid(const com_util_timespec *timestamp);

    /**
     *  @brief          トレース出力に使用するタイムスタンプを解決します。
     *  @param[in]      timestamp      呼び出し側が渡した明示タイムスタンプ。NULL 可。
     *  @param[out]     resolved       解決後のタイムスタンプ格納先。
     *  @param[out]     fallback_used  不正タイムスタンプから現在時刻へ代替した場合 1。NULL 可。
     *  @return         成功時 0、解決失敗時 -1。
     *
     *  timestamp が有効な場合はそれを、NULL または不正な場合は現在時刻を
     *  resolved へ格納します。不正な値からの代替時のみ fallback_used に 1 を
     *  設定します (NULL は代替と見なさない)。
     */
    int trace_resolve_timestamp(const com_util_timespec *timestamp, com_util_timespec *resolved, int *fallback_used);

    /**
     *  @brief          実時刻を ISO 8601 ローカル時刻文字列としてバッファーへ書き込みます。
     *  @param[out]     buf        書き込み先バッファー。
     *  @param[in]      buf_size   バッファーのバイト数 (COM_UTIL_CLOCK_ISO8601_LOCAL_MSEC_LEN + 1 以上を推奨)。
     *  @param[in]      timestamp  使用する実時刻。
     *  @return         成功時 0、タイムスタンプ不正または整形失敗時 -1。
     */
    int trace_format_local_timestamp(char *buf, size_t buf_size, const com_util_timespec *timestamp);

    /**
     *  @brief          トレース レベルをレベル文字に変換します。
     *  @param[in]      level  変換元のトレース レベル。
     *  @return         対応するレベル文字 ('C'/'E'/'W'/'I'/'V'/'D')。範囲外は 'D'。
     */
    char trace_level_char(com_util_trace_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* COM_UTIL_TRACE_COMMON_H */
