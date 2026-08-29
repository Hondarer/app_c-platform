/**
 *  @file           stub_trace_common.c
 *  @brief          トレース共通ヘルパーの内部 API を、呼び出し元の単体テスト向けに提供します。
 *
 *  trace_common.c のカバレッジは traceCommonTest が担う。
 *  本スタブはタイムスタンプ解決、ISO 8601 整形、レベル文字変換の契約だけを満たす。
 */

#include <cplat/trace/trace_common.h>

static int timestamp_is_valid(const cplat_timespec *timestamp)
{
    return timestamp != NULL && timestamp->tv_nsec >= 0 && timestamp->tv_nsec < 1000000000;
}

int trace_resolve_timestamp(const cplat_timespec *timestamp, cplat_timespec *resolved, int *fallback_used)
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
        if (timestamp_is_valid(timestamp))
        {
            *resolved = *timestamp;
            return 0;
        }
        if (fallback_used != NULL)
        {
            *fallback_used = 1;
        }
    }

    cplat_get_realtime(resolved);
    if (timestamp_is_valid(resolved))
    {
        return 0;
    }
    return -1;
}

int trace_format_local_timestamp(char *buf, const size_t buf_size, const cplat_timespec *timestamp)
{
    if (!timestamp_is_valid(timestamp))
    {
        return -1;
    }
    return cplat_format_realtime_iso8601_local(buf, buf_size, timestamp);
}

char trace_level_char(const cplat_trace_level level)
{
    switch (level)
    {
    case CPLAT_TRACE_LEVEL_CRITICAL:
        return 'C';
    case CPLAT_TRACE_LEVEL_ERROR:
        return 'E';
    case CPLAT_TRACE_LEVEL_WARNING:
        return 'W';
    case CPLAT_TRACE_LEVEL_INFO:
        return 'I';
    case CPLAT_TRACE_LEVEL_VERBOSE:
        return 'V';
    case CPLAT_TRACE_LEVEL_DEBUG:
        return 'D';
    case CPLAT_TRACE_LEVEL_NONE:
    default:
        return 'D';
    }
}
