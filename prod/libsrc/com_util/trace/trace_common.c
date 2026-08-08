/**
 *******************************************************************************
 *  @file           trace_common.c
 *  @brief          トレース機能の共通ヘルパー (内部共有) を実装します。
 *
 *  tracer 本体と各バックエンドで重複していたタイムスタンプ解決と
 *  トレース レベル表現の変換を、本ファイルの 1 実装に集約します。
 *******************************************************************************
 */

#include <com_util/trace/trace_common.h>

/**
 *  @brief          タイムスタンプが有効範囲か判定します。
 *  @param[in]      timestamp  判定対象のタイムスタンプです。NULL も指定できます。
 *  @return         有効な場合は 1、NULL または範囲外の場合は 0 を返します。
 */
static int trace_timestamp_is_valid(const com_util_timespec *timestamp)
{
    return timestamp != NULL && timestamp->tv_nsec >= 0 && timestamp->tv_nsec < 1000000000;
}

/* Doxygen コメントは、ヘッダーに記載 */

int trace_resolve_timestamp(const com_util_timespec *timestamp, com_util_timespec *resolved, int *fallback_used)
{
    if (resolved == NULL)
    {
        return -1;
    }
    if (fallback_used != NULL)
    {
        *fallback_used = 0;
    }

    if (timestamp != NULL)
    {
        if (trace_timestamp_is_valid(timestamp))
        {
            *resolved = *timestamp;
            return 0;
        }
        if (fallback_used != NULL)
        {
            *fallback_used = 1;
        }
    }

    com_util_get_realtime(resolved);
    if (trace_timestamp_is_valid(resolved))
    {
        return 0;
    }
    return -1;
}

/* Doxygen コメントは、ヘッダーに記載 */

int trace_format_local_timestamp(char *buf, const size_t buf_size, const com_util_timespec *timestamp)
{
    if (!trace_timestamp_is_valid(timestamp))
    {
        return -1;
    }
    return com_util_format_realtime_iso8601_local(buf, buf_size, timestamp);
}

/* Doxygen コメントは、ヘッダーに記載 */

char trace_level_char(const com_util_trace_level level)
{
    switch (level)
    {
    case COM_UTIL_TRACE_LEVEL_CRITICAL:
        return 'C';
    case COM_UTIL_TRACE_LEVEL_ERROR:
        return 'E';
    case COM_UTIL_TRACE_LEVEL_WARNING:
        return 'W';
    case COM_UTIL_TRACE_LEVEL_INFO:
        return 'I';
    case COM_UTIL_TRACE_LEVEL_VERBOSE:
        return 'V';
    case COM_UTIL_TRACE_LEVEL_DEBUG:
        return 'D';
    case COM_UTIL_TRACE_LEVEL_NONE:
    default:
        return 'D';
    }
}
